#include "core/Toaster.h"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "audio/AudioStage.h"
#include "audio/PcmWriter.h"
#include "audio/WavFileWriter.h"
#include "classification/ClassificationStage.h"
#include "classification/TerminalClassificationSink.h"
#include "core/StageSelection.h"
#include "dsp/filters/Filters.h"
#include "messaging/Messages.h"
#include "scanner/ScannerStage.h"
#include "scanner/TerminalScannerSink.h"
#include "spectrum/BarSpectrumRenderer.h"
#include "spectrum/SpectrumStage.h"
#include "spectrum/WaterfallRenderer.h"

namespace {

ReceiverConfig toReceiverConfig(const ToasterConfig& config) {
    ReceiverConfig receiverConfig;
    receiverConfig.centerFreqHz = config.centerFreqHz;
    receiverConfig.deviceIndex = config.deviceIndex;
    receiverConfig.manualGainTenthsDb = config.manualGainTenthsDb;
    receiverConfig.sampleRateHz = config.rfSampleRateHz;
    return receiverConfig;
}

AudioStageConfig toAudioStageConfig(const ToasterConfig& config) {
    AudioStageConfig audioStageConfig;
    audioStageConfig.rfSampleRateHz = config.rfSampleRateHz;
    audioStageConfig.audioSampleRateHz = config.audioSampleRateHz;
    audioStageConfig.deemphasisMicroseconds = config.deemphasisMicroseconds;
    audioStageConfig.postTuneMuteMs = config.audioPostTuneMuteMs;
    audioStageConfig.postTuneFadeInMs = config.audioPostTuneFadeInMs;
    return audioStageConfig;
}

SpectrumStageConfig toSpectrumStageConfig(const ToasterConfig& config) {
    SpectrumStageConfig spectrumStageConfig;
    spectrumStageConfig.centerFreqHz = config.centerFreqHz;
    spectrumStageConfig.rfSampleRateHz = config.rfSampleRateHz;
    spectrumStageConfig.fftSize = config.spectrumFftSize;
    spectrumStageConfig.refreshIntervalMs = config.spectrumRefreshIntervalMs;
    spectrumStageConfig.smoothingFactor = config.spectrumSmoothingFactor;
    spectrumStageConfig.minDb = config.spectrumMinDb;
    spectrumStageConfig.maxDb = config.spectrumMaxDb;
    spectrumStageConfig.maxDecimation = config.spectrumMaxDecimation;
    return spectrumStageConfig;
}

ClassificationStageConfig toClassificationStageConfig(const ToasterConfig& config) {
    ClassificationStageConfig classificationConfig;
    classificationConfig.centerFreqHz = config.centerFreqHz;
    classificationConfig.rfSampleRateHz = config.rfSampleRateHz;
    classificationConfig.fftSize = config.classificationFftSize;
    classificationConfig.refreshIntervalMs = config.classificationRefreshIntervalMs;
    classificationConfig.detectionMarginDb = config.classificationDetectionMarginDb;
    classificationConfig.minRunBins = config.classificationMinRunBins;
    classificationConfig.logPath = config.classificationLogPath;
    return classificationConfig;
}

ScannerStageConfig toScannerStageConfig(const ToasterConfig& config) {
    ScannerStageConfig scannerConfig;
    scannerConfig.startFreqHz = config.scanStartFreqHz;
    scannerConfig.endFreqHz = config.scanEndFreqHz;
    scannerConfig.coarseStepHz = config.scanCoarseStepHz;
    scannerConfig.fineStepHz = config.scanFineStepHz;
    scannerConfig.fineRangeHz = config.scanFineRangeHz;
    scannerConfig.settleMs = config.scanSettleMs;
    scannerConfig.measureMs = config.scanMeasureMs;
    scannerConfig.thresholdDb = config.scanThresholdDb;
    scannerConfig.lockMs = config.scanLockMs;
    scannerConfig.logPath = config.scanLogPath;
    return scannerConfig;
}

// Builds whichever anti-aliasing Filter the config asked for. This is
// the one place that decides FIR vs IIR — AudioStage itself just sees a
// Filter.
std::unique_ptr<AdjustableLowPassFilter> makeAntiAliasFilter(const ToasterConfig& config) {
    if (config.antiAliasFilterType == FilterImplementation::Iir) {
        return std::make_unique<IirLowPassFilter>(config.audioCutoffHz, config.rfSampleRateHz);
    }
    return std::make_unique<FirLowPassFilter>(config.firNumTaps, config.audioCutoffHz,
                                               config.rfSampleRateHz);
}

// Builds whichever AudioSink the config asked for. This is the one place
// that decides "raw PCM to stdout" vs. "a .wav file" — AudioStage itself
// just sees an AudioSink.
std::unique_ptr<AudioSink> makeSink(const ToasterConfig& config) {
    if (config.outputMode == AudioOutputMode::WavFile) {
        return std::make_unique<WavFileWriter>(config.outputWavPath, config.audioSampleRateHz,
                                                /*numChannels=*/1, config.outputGain);
    }
    return std::make_unique<PcmWriter>(stdout, config.outputGain);
}

// Builds whichever SpectrumSink the config asked for. This is the one
// place that decides bars vs. waterfall — SpectrumStage itself just
// sees a SpectrumSink, the same way AudioStage just sees an AudioSink.
//
// Both renderers are given stderr explicitly rather than relying on
// their default, to make the reason visible at the decision point:
// stdout belongs to the raw PCM audio stream, so a spectrum display
// written there would corrupt the audio of anyone running
// `-s audio,spectrum-visual | aplay`.
std::unique_ptr<SpectrumSink> makeSpectrumSink(const ToasterConfig& config) {
    if (config.spectrumRenderer == SpectrumRendererType::Waterfall) {
        return std::make_unique<WaterfallRenderer>(stderr);
    }
    return std::make_unique<BarSpectrumRenderer>(stderr);
}

// Builds the ClassificationSink the config asked for. Unlike
// makeSpectrumSink, there's only one implementation today
// (TerminalClassificationSink) -- this still exists as its own function
// so ClassificationStage never needs to know a concrete sink type, and
// so a future second implementation (a database-backed one, say) has an
// obvious, single place to be chosen from.
std::unique_ptr<ClassificationSink> makeClassificationSink(const ToasterConfig& config) {
    return std::make_unique<TerminalClassificationSink>(config.classificationLogPath);
}

std::unique_ptr<ScannerSink> makeScannerSink(const ToasterConfig& config) {
    return std::make_unique<TerminalScannerSink>(config.scanLogPath);
}

bool isSelected(const std::string& key, const std::vector<std::string>& selectedKeys) {
    return std::find(selectedKeys.begin(), selectedKeys.end(), key) != selectedKeys.end();
}

}  // namespace

std::vector<StageDescriptor> Toaster::allStageDescriptors() {
    // Every stage Toaster knows how to build is listed here, in the
    // order it's offered in the selection prompt. Each factory
    // subscribes its stage to whatever message type(s) it needs
    // directly with the given Dispatcher in the stage's own
    // constructor. Adding spectrum-visual below needed exactly one
    // entry here and nothing else: no change to Dispatcher, Receiver,
    // AudioStage, or Pipeline-anything, because both stages simply
    // subscribe to IQMessage and neither is aware the other exists.
    //
    // A further stage plugs in the same way, e.g.:
    //
    //   {"spectrum-electronic", "Electronic-spectrum analysis output",
    //    [](const ToasterConfig& config, Dispatcher& dispatcher) -> std::unique_ptr<PipelineStage> {
    //        return std::make_unique<ElectronicSpectrumStage>(dispatcher, ...);
    //    }},
    //
    // Because each entry is independently selectable (see
    // core/StageSelection.h), a user who doesn't want audio output and
    // a visual spectrum display running at the same time simply doesn't
    // select both -- nothing here forces them to run together.
    return {
        {"audio", "Demodulated audio out (stdout PCM or .wav)",
         [](const ToasterConfig& config, Dispatcher& dispatcher) -> std::unique_ptr<PipelineStage> {
             return std::make_unique<AudioStage>(toAudioStageConfig(config), dispatcher,
                                                  makeAntiAliasFilter(config), makeSink(config));
         }},
        {"spectrum-visual", "Live on-screen spectrum display (bars or waterfall)",
         [](const ToasterConfig& config, Dispatcher& dispatcher) -> std::unique_ptr<PipelineStage> {
             return std::make_unique<SpectrumStage>(toSpectrumStageConfig(config), dispatcher,
                                                     makeSpectrumSink(config));
         }},
        {"classification", "Feature-based signal detection + best-guess device match",
         [](const ToasterConfig& config, Dispatcher& dispatcher) -> std::unique_ptr<PipelineStage> {
             return std::make_unique<ClassificationStage>(toClassificationStageConfig(config), dispatcher,
                                                           makeClassificationSink(config));
         }},
        {"scanner", "Sweeps a frequency range for strong signals, locking on and letting audio listen",
         [](const ToasterConfig& config, Dispatcher& dispatcher) -> std::unique_ptr<PipelineStage> {
             return std::make_unique<ScannerStage>(toScannerStageConfig(config), dispatcher,
                                                    makeScannerSink(config));
         }},
    };
}

Toaster::Toaster(const ToasterConfig& config)
    : config_(config),
      // Receiver knows nothing about Dispatcher (see rf/Receiver.h) --
      // it just hands us samples. Forwarding them into an IQMessage on
      // dispatcher_ is the wiring Toaster exists to do. This lambda
      // runs synchronously on the RTL-SDR read thread, same as a direct
      // dispatcher_.dispatch() call from inside Receiver would have, so
      // there's no extra latency or risk of falling behind on samples.
      receiver_(toReceiverConfig(config),
                [this](const std::complex<float>* samples, size_t count) {
                    dispatcher_.dispatch(IQMessage{samples, count});
                }),
      controller_(dispatcher_, receiver_),
      console_(),
      commandListener_(console_, dispatcher_) {
    // Ask (via -s, or interactively on the console if -s wasn't given)
    // which of the known stages to actually build, then loop over all of
    // them building only the selected ones -- rather than a fixed
    // sequence of push_back() calls naming each stage individually. This
    // is what lets new stages (a spectrum analyzer, say) be added to
    // allStageDescriptors() and immediately become independently
    // optional, with no changes needed here.
    std::vector<StageDescriptor> descriptors = allStageDescriptors();
    std::vector<std::string> selectedKeys = resolveStages(console_, descriptors, config.requestedStageKeys);

    // If scanner is running without audio, there's no one listening
    // live to a lock -- holding it for lockMs (sized for a human to
    // actually hear something, 8s by default) would mean a scan-only
    // pass over a busy band spends most of its time sitting on hits
    // for no benefit. Swap in the much shorter quickConfirmMs instead,
    // unless the user explicitly asked for a specific lockMs (see
    // ToasterConfig::scanLockMsExplicit) -- an explicit request always
    // wins. This has to happen here, before descriptor.factory() builds
    // ScannerStage below, since ScannerStage itself has no idea what
    // else is selected (nor should it -- see ScannerStage.h) and just
    // uses whatever lockMs its config says.
    if (isSelected("scanner", selectedKeys) && !isSelected("audio", selectedKeys) &&
        !config_.scanLockMsExplicit) {
        config_.scanLockMs = config_.scanQuickConfirmMs;
    }

    for (const auto& descriptor : descriptors) {
        if (isSelected(descriptor.key, selectedKeys)) {
            stages_.push_back(descriptor.factory(config_, dispatcher_));
        }
    }
    selectedStageKeys_ = std::move(selectedKeys);
}

bool Toaster::run() {
    if (Receiver::getDeviceCount() == 0) {
        fprintf(stderr, "No RTL-SDR devices found.\n");
        return false;
    }
    fprintf(stderr, "Using device %d: %s\n", config_.deviceIndex,
            Receiver::getDeviceName(config_.deviceIndex).c_str());

    if (!receiver_.open()) {
        fprintf(stderr, "Failed to open RTL-SDR device %d\n", config_.deviceIndex);
        return false;
    }

    fprintf(stderr, "Tuner gain: %s\n", config_.manualGainTenthsDb < 0 ? "auto" : "manual");
    fprintf(stderr, "Tuned to %.3f MHz\n", config_.centerFreqHz / 1e6);

    // Each stage reports only if it's actually running: the anti-alias
    // filter and the PCM output format are facts about AudioStage, and
    // announcing them during a spectrum-only run would describe output
    // that isn't being produced.
    if (isSelected("audio", selectedStageKeys_)) {
        fprintf(stderr, "Anti-alias filter: %s low-pass, %.0f Hz cutoff (type 'bandwidth <hz>' to adjust live)\n",
                config_.antiAliasFilterType == FilterImplementation::Iir ? "IIR" : "FIR",
                config_.audioCutoffHz);
        if (config_.outputMode == AudioOutputMode::WavFile) {
            fprintf(stderr, "Writing 16-bit mono PCM @ %u Hz to '%s'...\n",
                    config_.audioSampleRateHz, config_.outputWavPath.c_str());
        } else {
            fprintf(stderr, "Streaming 16-bit mono PCM @ %u Hz to stdout...\n",
                    config_.audioSampleRateHz);
        }
    }
    if (isSelected("spectrum-visual", selectedStageKeys_)) {
        fprintf(stderr, "Spectrum: %d-bin %s, refreshing every %d ms...\n", config_.spectrumFftSize,
                config_.spectrumRenderer == SpectrumRendererType::Waterfall ? "waterfall" : "bar display",
                config_.spectrumRefreshIntervalMs);
    }
    if (isSelected("classification", selectedStageKeys_)) {
        fprintf(stderr, "Classification: %d-bin FFT, %.0f dB margin, logging to %s...\n",
                config_.classificationFftSize, config_.classificationDetectionMarginDb,
                config_.classificationLogPath == "-" ? "stderr" : config_.classificationLogPath.c_str());
    }
    if (isSelected("scanner", selectedStageKeys_)) {
        bool fineEnabled = config_.scanFineStepHz > 0.0 && config_.scanFineRangeHz > 0.0;
        fprintf(stderr,
                "Scanner: sweeping %.3f-%.3f MHz in %.0f kHz steps%s, threshold %.0f dBFS, "
                "locking for %d ms%s, logging to %s...\n",
                config_.scanStartFreqHz / 1e6, config_.scanEndFreqHz / 1e6, config_.scanCoarseStepHz / 1e3,
                fineEnabled ? " (+ fine refinement)" : "", config_.scanThresholdDb, config_.scanLockMs,
                (!isSelected("audio", selectedStageKeys_) && !config_.scanLockMsExplicit)
                    ? " (quick-confirm; no audio selected)"
                    : "",
                config_.scanLogPath == "-" ? "stderr" : config_.scanLogPath.c_str());
    }
    // spectrum-visual's renderer redraws in place using ANSI cursor
    // movement on stderr, on the assumption it owns everything below
    // where it last drew (see spectrum/TerminalSpectrumRenderer.h and
    // classification/TerminalClassificationSink.h's matching comment).
    // A "-" log path for classification/scanner writes plain lines to
    // that same stream, which will corrupt the display the next time it
    // redraws -- flagged here, once, rather than letting it show up as
    // a confusing garbled terminal with no explanation.
    if (isSelected("spectrum-visual", selectedStageKeys_) &&
        ((isSelected("classification", selectedStageKeys_) && config_.classificationLogPath == "-") ||
         (isSelected("scanner", selectedStageKeys_) && config_.scanLogPath == "-"))) {
        fprintf(stderr,
                "Warning: spectrum-visual's live display and a '-' (stderr) log will corrupt each "
                "other's output. Point --classification-log/--scan-log at a file instead, or drop "
                "spectrum-visual for this run.\n");
    }

    // Blocks until a "stop" CommandMessage reaches Controller, which
    // calls receiver_.stop(). The stop listener isn't started until the
    // first real IQMessage is dispatched (see the callback passed to
    // receiver_.run() below and the matching comment in Receiver.h) --
    // that's what guarantees rtlsdr_cancel_async() will actually do
    // something by the time "stop" can possibly be typed.
    receiver_.run([this]() { commandListener_.start(); });

    for (auto& stage : stages_) stage->flush();
    return true;
}

void Toaster::stop() { dispatcher_.dispatch(CommandMessage{"stop"}); }
