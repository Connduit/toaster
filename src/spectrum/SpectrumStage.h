#pragma once

#include <chrono>
#include <complex>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "core/Config.h"
#include "core/PipelineStage.h"
#include "dsp/DigitalDownConverter.h"
#include "dsp/Fft.h"
#include "messaging/Dispatcher.h"
#include "messaging/Messages.h"
#include "spectrum/SpectrumFrame.h"
#include "spectrum/SpectrumSink.h"
#include "spectrum/SpectrumView.h"

// A PipelineStage that turns IQMessages into a visual spectrum.
//
// Owns the IQ-to-spectrum chain: collect a window of samples -> apply a
// Hann window -> FFT -> magnitude in dBFS -> average with recent frames
// -> SpectrumSink. It subscribes itself to IQMessage on the Dispatcher
// from its own constructor, exactly as AudioStage does, and the two are
// completely independent: selecting both means the same IQMessage is
// delivered to each of them, and neither knows the other exists.
//
// Note what this stage does *not* do: demodulate. AudioStage's chain
// starts with FmDemodulator because it wants the information encoded on
// the carrier; this one deliberately works on the raw IQ, because the
// question it answers is "what is present across this captured band of
// spectrum" -- which is a property of the RF itself, and would be
// destroyed by demodulating to a single station's audio first.
//
// ## Why most of the data is thrown away
//
// At the default 1.92 MS/s a 1024-point window fills roughly 1875 times
// a second (proportionally more at a higher rfSampleRateHz).
// Transforming all of them would be pointless: a display refreshing
// faster than ~30 times a second shows a human nothing more, and the
// work happens on the RTL-SDR read thread, where overrunning the time
// budget means dropped USB transfers and, if the audio stage is also
// running, audible glitches. So the stage stays idle until the refresh
// interval has elapsed, collects exactly one window, transforms it, and
// goes back to idle -- transforming perhaps 20 windows a second and
// discarding the other ~1855.
//
// The samples skipped aren't a measurement loss in any meaningful
// sense: each window is an unbiased look at a signal whose statistics
// are essentially constant over the milliseconds involved, which is
// exactly what the frame averaging below smooths across. What it does
// mean is that this stage cannot see events shorter than the refresh
// interval -- fine for a display, wrong for anything that must not miss
// a burst. A stage with that requirement would want to consume every
// window, and would want to do it off the RF thread.
//
// ## Zoom and pan
//
// The stage subscribes to CommandMessage as well, so "zoom in" or
// "center 100.3M" typed at the console reach it the same way "stop"
// reaches Controller. Zooming runs the IQ through a
// DigitalDownConverter first, which narrows the span *and* improves
// resolution proportionally -- see that class for why this is a real
// zoom rather than magnifying the same bins.
//
// The view only ever selects a slice of what the dongle already
// captured; it never retunes the hardware itself. That keeps the
// display independent of the audio stage, which may be listening to a
// station through the same tuner. The cost is that the view cannot
// leave the captured band (rfSampleRateHz, 1.92 MHz by default -- see
// -r in main.cpp to capture more), and commands asking it to are
// clamped.
//
// Something else entirely -- ScannerStage -- can retune the hardware,
// via Controller (see Controller.cpp). This stage never asks for that,
// but it does listen for the resulting "tune <hz>" CommandMessage, the
// same broadcast way Controller does, purely to keep its own frequency
// label correct and to drop the now-stale average rather than smearing
// it into the new frequency's first few frames.
//
// ## Threading
//
// Sample handling runs synchronously on the RF read thread, because
// Dispatcher invokes handlers on whichever thread dispatched. That is
// safe *because* of the throttling above: one FFT plus one terminal
// write, a few dozen times a second, is a small fraction of the time
// budget between USB transfers.
//
// Commands are the exception: they arrive on the console thread, so a
// command handler that changed the view directly would be mutating
// filters and buffers while the RF thread was reading them. Instead the
// command handler only records what was asked for, under a mutex, and
// the RF thread picks it up at a window boundary -- the one moment when
// no window is half-collected and rebuilding the downconverter is
// harmless. The mutex is taken twice per frame rather than per sample,
// so its cost is irrelevant. If a future renderer is slow enough to
// matter (a remote display over a slow link, say), the fix is to hand
// finished frames to a dedicated render thread through a single-slot
// mailbox that drops rather than queues -- for a display, the newest
// frame is the only one worth having. That change would be contained
// entirely within this class.
class SpectrumStage : public PipelineStage {
public:
    // Takes ownership of `sink`, which receives one finished frame per
    // refresh interval. Registers itself for IQMessage on `dispatcher`
    // immediately.
    SpectrumStage(const SpectrumStageConfig& config, Dispatcher& dispatcher,
                  std::unique_ptr<SpectrumSink> sink);

    void flush() override;
    void reset() override;

private:
    void onIQMessage(const IQMessage& message);

    // Runs on the console thread. Records a requested view; does not
    // touch any DSP state. See the threading note above.
    void onCommand(const CommandMessage& command);

    // The view commands should compose against: the last requested one
    // if the RF thread hasn't adopted it yet, otherwise the live one.
    SpectrumView currentRequestedView();

    // Runs on the RF thread, at a window boundary only. Adopts whatever
    // the console thread last asked for and reconfigures the
    // downconverter to match, and adopts any pending retune (see
    // tuneRequestPending_ below) by updating config_.centerFreqHz and
    // discarding the stale average -- both for the same reason and at
    // the same moment, since a window boundary is the only safe time to
    // touch either.
    void applyPendingView();

    // Begins collecting a fresh window: resets the downconverter and
    // arms the warm-up discard.
    void beginWindow();

    // Copies as much of `samples` into the window as still fits,
    // discarding warm-up samples first. Returns true once the window is
    // full.
    bool fillWindow(const std::complex<float>* samples, size_t count);

    // True once the refresh interval has elapsed since the last frame.
    bool frameDue() const;

    // Window -> FFT -> dB -> average -> sink, for the samples currently
    // sitting in fftBuffer_.
    void processWindow();

    SpectrumStageConfig config_;
    Fft fft_;

    // The slice currently being displayed, and the machinery that
    // extracts it. Both are touched only by the RF thread.
    SpectrumView view_;
    DigitalDownConverter downConverter_;
    std::vector<std::complex<float>> decimated_;
    size_t warmUpRemaining_ = 0;

    // Written by the console thread, read by the RF thread. Guarded
    // because a SpectrumView is two fields and a torn read of it would
    // configure a span and an offset that were never asked for together.
    std::mutex viewMutex_;
    SpectrumView requestedView_;
    bool viewRequestPending_ = false;

    // A pending retune, guarded by the same viewMutex_ and applied at
    // the same window boundary as a pending view, for the same reason:
    // "tune <hz>" can arrive from a different thread than the one
    // running onIQMessage (see messaging/Messages.h's note on
    // CommandMessage -- today it's always ScannerStage on the RF thread
    // itself, but nothing here assumes that will always be true, so
    // this is treated with the same care as a console-typed "zoom").
    double requestedCenterFreqHz_ = 0.0;
    bool tuneRequestPending_ = false;

    // Feedback for the most recent command, and when it arrived.
    // Cleared after a few seconds: it describes something that just
    // happened, and leaving it pinned to the header forever would both
    // mislead and permanently cost the readout the space it occupies.
    std::string statusMessage_;
    std::chrono::steady_clock::time_point statusSetAt_;

    // Hann coefficients and the reciprocal of their sum squared, which
    // is the normalization that puts a full-scale complex tone at
    // 0 dBFS regardless of window length. Precomputed because it's
    // fixed for the life of the stage.
    std::vector<float> windowCoefficients_;
    float powerNormalization_ = 1.0f;

    // Samples for the window currently being collected; transformed in
    // place. `collecting_` distinguishes "part-way through gathering a
    // window" from "idle, waiting for the next refresh" -- without it,
    // a window straddling two IQMessages would be abandoned halfway.
    std::vector<std::complex<float>> fftBuffer_;
    size_t fill_ = 0;
    bool collecting_ = false;

    // Frame averaging state, in linear power, FFT-shifted so index 0 is
    // the lowest displayed frequency.
    std::vector<float> averagedPower_;
    std::vector<float> magnitudesDb_;
    bool haveAverage_ = false;

    std::chrono::steady_clock::time_point lastFrameTime_;
    std::unique_ptr<SpectrumSink> sink_;
};
