#pragma once

#include <chrono>
#include <memory>
#include <vector>

#include "core/Config.h"
#include "core/PipelineStage.h"
#include "messaging/Dispatcher.h"
#include "messaging/Messages.h"
#include "scanner/ScannerSink.h"

// A PipelineStage that sweeps a frequency range looking for strong
// signals, classic-scanner-radio style: tune to a channel, measure
// broadband RSSI directly from the raw IQ (no FFT -- see below), and
// either move on or lock and let whatever's listening (AudioStage,
// ClassificationStage, ...) have it for a while.
//
// ## Coarse sweep, then fine refinement
//
// A single sweep granularity is a bad fit for "survey a wide range,
// then pin down exactly what's there": fine enough to localize a
// signal precisely makes a wide survey painfully slow, and coarse
// enough to survey quickly can land anywhere within a signal's width --
// including its edge, well off its actual peak. So this stage does
// both, automatically: it sweeps config.startFreqHz..endFreqHz in
// config.coarseStepHz increments (see SweepMode::Coarse), and the
// moment a coarse step measures above config.thresholdDb, it switches
// to SweepMode::Fine -- a short sub-sweep of config.fineStepHz-spaced
// candidates across config.fineRangeHz centered on that coarse hit,
// each measured the same way. Once every fine candidate has been
// measured, it retunes to whichever one measured strongest and locks
// there -- not necessarily the original coarse-grid frequency, and not
// necessarily the last fine candidate measured either. Locking always
// happens at the best point found, coarse or fine. Setting
// config.fineStepHz (or config.fineRangeHz) to 0 disables fine
// refinement entirely, restoring the simpler "lock exactly on the
// coarse hit" behavior.
//
// Finishing a fine refinement (locked or not) resumes the *coarse*
// sweep one coarseStepHz past wherever the refinement started, not from
// any fine candidate's position -- otherwise consecutive coarse steps
// landing near the same signal could refine it repeatedly instead of
// moving on.
//
// ## Why RSSI, not an FFT
//
// SpectrumStage and ClassificationStage both need to know *where within
// a captured band* a signal sits, so they FFT. ScannerStage doesn't:
// it retunes to each candidate frequency in turn and asks only "is
// there something strong right here", which total signal power answers
// directly and far more cheaply than a transform would -- true at both
// sweep granularities. This also keeps ScannerStage entirely
// independent of dsp/Fft.h and dsp/WindowFunction.h -- one less thing
// coupling it to how the other spectrum-facing stages happen to work.
//
// ## Retuning: the only stage that does
//
// Every other stage in this project deliberately never retunes the
// hardware (see spectrum/SpectrumStage.h's "Zoom and pan" section for
// why). ScannerStage is the one exception, because sweeping a range
// wider than one captured band *requires* it. It doesn't touch Receiver
// directly, though: it dispatches "tune <hz>" as a CommandMessage (see
// messaging/Messages.h) the same way a console-typed "zoom" would reach
// SpectrumStage, and Controller -- which already owns Receiver -- is
// what actually calls Receiver::retune() (see Controller.cpp and its
// "Retuning" section on why that itself has to happen off the RF
// thread). This stage doesn't know Controller exists, or that
// AudioStage/SpectrumStage/ClassificationStage are listening for the
// same broadcast to relabel, reset, or mute themselves; the "lock and
// listen" handoff described in this stage's header therefore requires
// no coupling to any of them at all.
//
// ## Threading
//
// Every method here runs on the RF read thread: onIQMessage() is
// invoked there by Dispatcher, and it's the only entry point into this
// class's state (unlike SpectrumStage/ClassificationStage, this stage
// doesn't subscribe to CommandMessage itself, so there's no
// console-thread entry point to guard against). No mutex is needed as a
// result.
class ScannerStage : public PipelineStage {
public:
    // Takes ownership of `sink`, which receives a report() for every
    // lock/resume transition. Registers itself for IQMessage on
    // `dispatcher` immediately, and keeps a reference to dispatch
    // "tune <hz>" CommandMessages and ScanEventMessages as sweeping
    // proceeds.
    ScannerStage(const ScannerStageConfig& config, Dispatcher& dispatcher,
                 std::unique_ptr<ScannerSink> sink);

    void flush() override;
    void reset() override;

private:
    enum class Phase {
        Settling,   // just retuned; discarding samples until the tuner's PLL has settled
        Measuring,  // accumulating power to decide whether this channel is busy
        Locked,     // sitting on a hit, letting whatever's listening have it
    };

    enum class SweepMode {
        Coarse,  // surveying the full range in coarseStepHz increments
        Fine,    // refining around a coarse hit in fineStepHz increments
    };

    void onIQMessage(const IQMessage& message);

    void enterSettling();
    void enterMeasuring();
    void enterLocked(float powerDb);
    void resumeSweep();

    // Called once a Measuring window's power reading is known. Decides
    // what happens next: advance the coarse sweep, start fine
    // refinement, advance within it, or lock -- see this class's header
    // comment for the full state machine.
    void handleMeasurementComplete(float powerDb);

    // Retunes the shared hardware to `freqHz` by dispatching
    // "tune <hz>" (see this class's header comment).
    void retuneTo(double freqHz);

    // Steps coarseFreqHz_ by config_.coarseStepHz, wrapping back to
    // config_.startFreqHz past config_.endFreqHz -- the sweep loops
    // rather than stopping at the end of the range.
    void advanceToNextCoarseFrequency();

    // Builds fineCandidates_ around `centerHz` (a coarse hit) and
    // switches into SweepMode::Fine, retuning to the first candidate.
    // `coarsePowerDb` seeds fineBestPowerDb_/fineBestFreqHz_ so the
    // coarse hit itself is never worse than what's found -- refinement
    // can only do better, never worse, than just locking on the coarse
    // hit directly.
    void beginFineRefinement(double centerHz, float coarsePowerDb);

    ScannerStageConfig config_;
    Dispatcher& dispatcher_;
    std::unique_ptr<ScannerSink> sink_;

    Phase phase_ = Phase::Settling;
    SweepMode mode_ = SweepMode::Coarse;

    double currentFreqHz_ = 0.0;  // wherever the hardware is physically tuned right now
    double coarseFreqHz_ = 0.0;   // the coarse sweep's own bookmark, untouched during fine refinement

    std::vector<double> fineCandidates_;
    size_t fineIndex_ = 0;
    double fineBestFreqHz_ = 0.0;
    float fineBestPowerDb_ = -200.0f;

    // Set when finishing fine refinement requires one more retune (to
    // the best candidate found, if it wasn't the last one measured)
    // before actually locking -- see handleMeasurementComplete(). Tells
    // the next Settling deadline to go straight to enterLocked() rather
    // than enterMeasuring(), since the power at that exact frequency is
    // already known from the fine sweep and doesn't need re-measuring.
    bool pendingLockAfterSettle_ = false;

    // The very first onIQMessage() call reflects whatever frequency the
    // receiver happened to open on (main.cpp's -f), not the sweep's
    // start -- this triggers the first real retune and is then never
    // touched again. Without it, the sweep's first "measurement" would
    // actually be measuring -f's frequency, not config_.startFreqHz's.
    bool started_ = false;

    std::chrono::steady_clock::time_point phaseDeadline_;

    // Accumulated during Measuring only; reset at the start of each
    // measurement window (see enterMeasuring()).
    double powerSum_ = 0.0;
    size_t powerCount_ = 0;

    // The power reading that caused the most recent lock, kept so
    // resumeSweep() can report it alongside the frequency being left --
    // there's no fresh measurement to report at that moment, since
    // nothing is measured while locked.
    float lastLockPowerDb_ = 0.0f;

    // A low-rate "still scanning, now at X" line so a long quiet sweep
    // doesn't look identical to a hung program. Not tied to
    // config -- unlike the sweep's own timing, this is purely cosmetic
    // and a fixed interval is plenty.
    std::chrono::steady_clock::time_point lastHeartbeat_;
};
