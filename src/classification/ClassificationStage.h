#pragma once

#include <chrono>
#include <complex>
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

#include "classification/ClassificationSink.h"
#include "core/Config.h"
#include "core/PipelineStage.h"
#include "dsp/Fft.h"
#include "messaging/Dispatcher.h"
#include "messaging/Messages.h"

// A PipelineStage that looks for signals in the captured RF band and
// makes a feature-based guess at what kind of device each one is,
// using nothing but its measured center frequency and bandwidth
// against a small built-in table (see classification/DeviceCatalog.h).
// There is no machine learning here, by design -- see DeviceCatalog.h
// for why that's a deliberate choice for now, not a placeholder for one
// that's missing.
//
// Owns its own, entirely independent window -> Hann -> FFT -> dB chain,
// duplicating the shape of SpectrumStage's rather than subscribing to
// anything SpectrumStage produces. That's deliberate: SpectrumStage's
// view can be zoomed/panned by the user at any moment (see
// spectrum/SpectrumView.h), and classification needs to see the whole
// captured band regardless of whatever the display currently shows --
// coupling to SpectrumStage's frames would mean a zoomed-in display
// silently blinding the classifier to everything outside the current
// view. The duplicated FFT work this costs is the same trade
// SpectrumStage itself already makes (see its header comment on "why
// most of the data is thrown away"): a modest transform a few times a
// second is nowhere near the RF thread's time budget.
//
// Subscribes itself to IQMessage and CommandMessage on the Dispatcher
// from its own constructor, exactly like every other stage. It also
// *dispatches* a SignalDetectionMessage for every signal it finds (in
// addition to handing the same struct to its injected ClassificationSink
// -- see that class), which is the "publish/subscribe" half of this
// stage's design: any future stage that wants to react to a detection
// (an alerting stage, say, or a stage that logs detections to a
// database) subscribes to SignalDetectionMessage without
// ClassificationStage ever needing to know it exists, the same way nothing
// producing an IQMessage needs to know AudioStage or SpectrumStage exist.
//
// Selecting this alongside "scanner" and "audio" composes for free, with
// zero direct coupling between the three: ScannerStage retunes the
// shared hardware when it finds a strong signal (see scanner/
// ScannerStage.h), AudioStage demodulates whatever's now on the air
// because it processes every IQMessage regardless of frequency, and
// this stage independently starts reporting features for the new
// frequency the moment its own next window fills -- "lock, listen, and
// identify" emerges from three stages that have never heard of each
// other, all reacting to the same shared tuner and the same
// "tune <hz>" CommandMessage.
class ClassificationStage : public PipelineStage {
public:
    // Takes ownership of `sink`, which receives one report() per
    // detected signal. Registers itself for IQMessage and CommandMessage
    // on `dispatcher` immediately, and keeps a reference to it to
    // dispatch SignalDetectionMessage later (see this class's comment).
    ClassificationStage(const ClassificationStageConfig& config, Dispatcher& dispatcher,
                         std::unique_ptr<ClassificationSink> sink);

    void flush() override;
    void reset() override;

private:
    void onIQMessage(const IQMessage& message);

    // Runs on whichever thread dispatched the CommandMessage (today,
    // always the RF thread -- see messaging/Messages.h's note on
    // CommandMessage). Only records a pending retune, under a mutex; see
    // applyPendingTune().
    void onCommand(const CommandMessage& command);

    // True once the refresh interval has elapsed since the last window.
    bool frameDue() const;

    // Adopts a pending retune (if any) at a window boundary -- the only
    // moment it's safe to touch config_.centerFreqHz, mirroring
    // SpectrumStage::applyPendingView for the same reason.
    void applyPendingTune();

    void beginWindow();

    // Copies as much of `samples` into the window as still fits.
    // Returns true once the window is full.
    bool fillWindow(const std::complex<float>* samples, size_t count);

    // Window -> FFT -> dB -> detect -> classify -> report, for the
    // samples currently sitting in fftBuffer_.
    void processWindow();

    // A robust estimate of the noise floor across the current window,
    // in dB: the ~20th-percentile bin value, found in place via
    // nth_element so this costs O(n) rather than a full sort. A
    // percentile rather than a mean because a mean is dragged upward by
    // any strong signal present -- exactly the thing detection is
    // trying to find -- while a low percentile stays close to the
    // actual noise floor as long as fewer than that percentile's worth
    // of bins are occupied by signal, true for nearly any real capture.
    float estimateNoiseFloorDb();

    // Turns one contiguous run of bins [runStart, runEnd) that crossed
    // the detection threshold into a SignalDetectionMessage: a
    // power-weighted centroid for the center frequency (more accurate
    // than the peak bin alone when a signal isn't perfectly
    // bin-aligned), the run's width in Hz for bandwidth, and a catalog
    // match. Reports it to sink_ and dispatches it on dispatcher_.
    void emitDetection(size_t runStart, size_t runEnd, double binWidthHz);

    ClassificationStageConfig config_;
    Fft fft_;
    Dispatcher& dispatcher_;

    std::vector<float> windowCoefficients_;
    float powerNormalization_ = 1.0f;

    std::vector<std::complex<float>> fftBuffer_;
    size_t fill_ = 0;
    bool collecting_ = false;

    // Scratch buffers, sized once and reused every window rather than
    // reallocated -- magnitudesDb_ is FFT-shifted (index 0 is the
    // lowest displayed frequency); noiseFloorScratch_ is a working copy
    // estimateNoiseFloorDb() partially reorders via nth_element.
    std::vector<float> magnitudesDb_;
    std::vector<float> noiseFloorScratch_;

    // Guards a pending retune the same way SpectrumStage's viewMutex_
    // guards a pending view -- see applyPendingTune().
    std::mutex tuneMutex_;
    double requestedCenterFreqHz_ = 0.0;
    bool tuneRequestPending_ = false;

    std::chrono::steady_clock::time_point lastFrameTime_;
    std::unique_ptr<ClassificationSink> sink_;
};
