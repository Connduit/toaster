#include "audio/AudioStage.h"

#include <algorithm>
#include <string>
#include <utility>

#include "spectrum/SpectrumCommands.h"

namespace {
constexpr const char* kTunePrefix = "tune ";
constexpr const char* kBandwidthPrefix = "bandwidth ";
}  // namespace

AudioStage::AudioStage(const AudioStageConfig& config, Dispatcher& dispatcher,
                        std::unique_ptr<AdjustableLowPassFilter> antiAliasFilter,
                        std::unique_ptr<AudioSink> sink)
    : config_(config),
      antiAliasFilter_(std::move(antiAliasFilter)),
      decimator_(static_cast<int>(config.rfSampleRateHz / config.audioSampleRateHz)),
      deemphasis_(config.audioSampleRateHz, config.deemphasisMicroseconds),
      sink_(std::move(sink)) {
    fadeInSampleCount_ =
        static_cast<size_t>(std::max(1.0, config.audioSampleRateHz * config.postTuneFadeInMs / 1000.0));

    dispatcher.registerTypedHandler<IQMessage>(
        [this](const IQMessage& message) { onIQMessage(message); });
    dispatcher.registerTypedHandler<CommandMessage>(
        [this](const CommandMessage& command) { onCommand(command); });
}

bool AudioStage::muted() const { return std::chrono::steady_clock::now() < muteUntil_; }

void AudioStage::onIQMessage(const IQMessage& message) {
    // Checked once per buffer, not per sample -- a steady_clock::now()
    // call for every one of up to ~2 million samples/sec would be far
    // too expensive. The imprecision this trades away is at most one
    // buffer's worth of time (well under a millisecond at typical
    // rfSampleRateHz), negligible for muting purposes.
    const bool isMuted = muted();

    for (size_t n = 0; n < message.count; ++n) {
        float decimated = 0.0f;
        bool haveDecimatedSample;

        if (isMuted) {
            // Deliberately skip the demodulator and anti-alias filter
            // entirely rather than feeding them silence: their history
            // was just reset() (see onCommand), and leaving them
            // untouched means real processing resumes from that clean
            // state the instant the mute ends, with nothing in between
            // to unwind. The decimator still runs (on a silent 0.0f)
            // so the audio stream's sample count -- and therefore its
            // real-time duration -- doesn't drift just because some of
            // it was silence.
            haveDecimatedSample = decimator_.push(0.0f, decimated);
        } else {
            float demodulated = demodulator_.demodulate(message.samples[n]);
            float filtered = antiAliasFilter_->process(demodulated);
            haveDecimatedSample = decimator_.push(filtered, decimated);
        }

        if (!haveDecimatedSample) continue;

        float sample;
        if (isMuted) {
            sample = 0.0f;  // true silence; deemphasis_ is left untouched, same reasoning as above
        } else {
            sample = deemphasis_.process(decimated);
            if (fadeInSamplesRemaining_ > 0) {
                // Ramp 0 -> 1 linearly over the first fadeInSampleCount_
                // audio-rate samples after a mute ends, rather than
                // snapping straight to full amplitude -- jumping from
                // true silence to full-amplitude audio in a single
                // 1/48000s sample is itself a small but audible edge.
                float gain = 1.0f - static_cast<float>(fadeInSamplesRemaining_) /
                                        static_cast<float>(fadeInSampleCount_);
                sample *= gain;
                --fadeInSamplesRemaining_;
            }
        }
        sink_->pushSample(sample);
    }
    sink_->flush();
}

void AudioStage::onCommand(const CommandMessage& command) {
    if (command.name.rfind(kTunePrefix, 0) == 0) {
        // Every other command (zoom/center/stop/help/bandwidth) is
        // nothing to do with a retune; only this one leaves DSP state
        // stale and IQ samples briefly untrustworthy. See this class's
        // header comment for why "tune" is handled the way it is here.
        reset();
        if (config_.postTuneMuteMs > 0) {
            muteUntil_ =
                std::chrono::steady_clock::now() + std::chrono::milliseconds(config_.postTuneMuteMs);
            fadeInSamplesRemaining_ = fadeInSampleCount_;
        }
        return;
    }

    if (command.name.rfind(kBandwidthPrefix, 0) == 0) {
        // Reuses spectrum_commands::parseFrequency purely as a
        // stateless string-to-Hz parser (see Controller.cpp's identical
        // reasoning) -- audio bandwidth values are a lot smaller than
        // the RF frequencies that parser was written for, but the
        // parsing itself (plain numbers, k/M/G suffixes, no dependency
        // on a dispatcher or a stage) is exactly as useful here.
        std::string argument = command.name.substr(std::string(kBandwidthPrefix).size());
        double cutoffHz = 0.0;
        bool isRelative = false;
        if (!spectrum_commands::parseFrequency(argument, cutoffHz, isRelative) || isRelative) return;

        // A cutoff at or above Nyquist for the *decimated* audio rate
        // isn't a narrower or wider filter, it's a filter that no
        // longer does its one job (preventing decimation from folding
        // energy above that point back down into the audible band) --
        // rejected rather than silently clamped, so a typo (an extra
        // zero, say) doesn't quietly disable anti-aliasing instead of
        // erroring.
        double nyquistHz = config_.audioSampleRateHz / 2.0;
        if (cutoffHz <= 0.0 || cutoffHz >= nyquistHz) return;

        antiAliasFilter_->setCutoffHz(cutoffHz);
    }
}

void AudioStage::flush() { sink_->flush(); }

void AudioStage::reset() {
    demodulator_.reset();
    antiAliasFilter_->reset();
    decimator_.reset();
    deemphasis_.reset();
    muteUntil_ = std::chrono::steady_clock::time_point{};
    fadeInSamplesRemaining_ = 0;
}
