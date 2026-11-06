#pragma once

#include <chrono>
#include <complex>
#include <cstdint>
#include <memory>

#include "audio/AudioSink.h"
#include "audio/FmDemodulator.h"
#include "core/Config.h"
#include "core/PipelineStage.h"
#include "dsp/filters/AdjustableLowPassFilter.h"
#include "dsp/filters/Decimator.h"
#include "dsp/filters/DeemphasisFilter.h"
#include "messaging/Dispatcher.h"
#include "messaging/Messages.h"

// A PipelineStage that turns IQMessages into audio.
//
// Owns the full IQ-to-audio DSP chain: FmDemodulator -> Filter (anti-
// aliasing) -> Decimator -> DeemphasisFilter -> AudioSink. Subscribes
// itself to IQMessage on the Dispatcher from its own constructor,
// rather than exposing a uniform processBuffer() a Pipeline used to
// call on every stage. A future stage that wants different data (say,
// AudioMessage, or a message type that doesn't exist yet) subscribes
// to whatever it needs the exact same way, with no changes needed here.
//
// Also subscribes to CommandMessage, for two reasons:
//
// - "tune <hz>" (see messaging/Messages.h). A hardware retune (from
//   ScannerStage sweeping, say) isn't just a phase discontinuity in the
//   IQ stream -- the RTL-SDR's tuner PLL takes a real, if short, amount
//   of time to relock afterward, and whatever arrives during that
//   window is closer to noise than signal. Demodulating it produces an
//   audible click, and it's the dominant source of clicking during an
//   active sweep: every hop that doesn't lock still retunes, so without
//   handling this every single hop clicks. reset()'ing the DSP chain's
//   history alone doesn't fix this -- it clears stale state, but does
//   nothing about genuinely bad samples still arriving after that. So a
//   "tune" instead starts a short mute window (see postTuneMuteMs):
//   during it, silence is pushed to the sink (at the correct decimated
//   rate, so audio timing doesn't drift) instead of demodulating
//   anything, and the DSP chain's history is reset so real processing
//   resumes from a clean state the moment the mute ends. A short linear
//   fade-in (postTuneFadeInMs) softens the silence-to-audio transition
//   at the end of the mute window, since jumping straight from true
//   silence to full-amplitude audio in one sample is itself a small but
//   audible edge.
// - "bandwidth <hz>", a live, gqrx-style audio filter width control:
//   sets the anti-aliasing filter's cutoff without needing to stop or
//   restart anything. This is why that filter is typed as
//   AdjustableLowPassFilter rather than the more general Filter --
//   AudioStage's anti-alias filter is always conceptually "a low-pass
//   with some cutoff" (see that interface's header comment), and this
//   command is the reason that cutoff needs to be reachable after
//   construction.
//
// The anti-aliasing filter and the sink are both injected as abstract
// interfaces (AdjustableLowPassFilter, AudioSink) rather than owned as
// concrete types -- Toaster decides FIR vs IIR and stdout vs .wav file;
// this class just uses whatever it's handed.
class AudioStage : public PipelineStage {
public:
    // Takes ownership of antiAliasFilter (run on every sample, before
    // decimation) and sink (receives every post-deemphasis sample).
    // Registers itself for IQMessage and CommandMessage on `dispatcher`
    // immediately.
    AudioStage(const AudioStageConfig& config, Dispatcher& dispatcher,
               std::unique_ptr<AdjustableLowPassFilter> antiAliasFilter, std::unique_ptr<AudioSink> sink);

    void flush() override;
    void reset() override;

private:
    void onIQMessage(const IQMessage& message);
    void onCommand(const CommandMessage& command);

    // True if still within postTuneMuteMs of the last "tune" command.
    bool muted() const;

    AudioStageConfig config_;
    FmDemodulator demodulator_;
    std::unique_ptr<AdjustableLowPassFilter> antiAliasFilter_;
    Decimator decimator_;
    DeemphasisFilter deemphasis_;
    std::unique_ptr<AudioSink> sink_;

    std::chrono::steady_clock::time_point muteUntil_;  // in the past initially -- see reset()

    // Counts down over the first fadeInSampleCount_ *audio-rate* (i.e.
    // post-decimation) samples after a mute window ends, linearly
    // ramping gain from 0 back to 1 rather than snapping straight to
    // full amplitude.
    size_t fadeInSampleCount_ = 0;
    size_t fadeInSamplesRemaining_ = 0;
};
