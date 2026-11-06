#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Configuration needed to receive raw IQ from a specific RTL-SDR device
// at a specific frequency. Used by Receiver.
struct ReceiverConfig {
    double centerFreqHz = 0.0;
    int deviceIndex = 0;
    int manualGainTenthsDb = -1;  // -1 means auto gain
    // How much RF bandwidth is captured. This is also the ceiling on
    // every other stage's view of the signal: SpectrumView can zoom out
    // to at most this much span, since the spectrum display only ever
    // slices what was actually captured rather than retuning the dongle
    // (see spectrum/SpectrumView.h). Settable via -r; must be a rate
    // librtlsdr's RTL2832U supports (225001-300000 or 900001-3200000).
    uint32_t sampleRateHz = 1920000;
};

// Configuration for the IQ-to-audio DSP chain. Used by AudioStage.
// Deliberately has no opinion about *which* anti-aliasing filter is used
// (FIR vs IIR — that's chosen and built externally, see Toaster) or
// *where* the resulting audio goes (an AudioSink, also built externally).
struct AudioStageConfig {
    // Must divide evenly by audioSampleRateHz for the decimator below to
    // hit exactly audioSampleRateHz -- see AudioStage's constructor,
    // which truncates the ratio to an int. main.cpp's -r flag warns
    // (without blocking) if the value it's given doesn't.
    uint32_t rfSampleRateHz = 1920000;
    uint32_t audioSampleRateHz = 48000;
    double deemphasisMicroseconds = 75.0;  // 75us US, 50us EU/most of world

    // How long to output silence after a "tune <hz>" command, instead of
    // demodulating the RTL-SDR's PLL-settling transient (see
    // AudioStage.h's class comment for why that transient is audible as
    // a click). 0 disables muting entirely, restoring the old
    // reset()-only behavior. How long a real dongle's PLL actually
    // takes to settle varies; if clicks are still audible during a
    // sweep, try raising this before anything else.
    int postTuneMuteMs = 20;

    // How long, after a mute ends, to linearly ramp gain from 0 back to
    // 1 rather than snapping straight to full amplitude. 0 disables the
    // ramp (an instant jump back to full volume).
    int postTuneFadeInMs = 15;
};

// Configuration for the IQ-to-spectrum analysis chain. Used by
// SpectrumStage. Like AudioStageConfig, it has no opinion about *where*
// the result is displayed -- that's a SpectrumSink, built externally by
// Toaster.
struct SpectrumStageConfig {
    // Needed to label the frequency axis: the FFT alone only knows
    // offsets from the tuned frequency, not what it's tuned to.
    double centerFreqHz = 0.0;

    // Also the widest span the spectrum view can ever show: "zoom out"
    // /"zoom reset" bottom out at decimation 1, i.e. this whole captured
    // band, since the view never retunes the dongle to look beyond it
    // (see spectrum/SpectrumView.h). Raise this (via -r) to be able to
    // zoom out further; there is no other way to.
    uint32_t rfSampleRateHz = 1920000;

    // Number of FFT bins across the displayed span. Must be a power of
    // two (Fft rounds up if it isn't). Bigger means finer frequency
    // resolution but a longer window, so less time resolution and more
    // work per frame.
    int fftSize = 1024;

    // How often a frame is computed and drawn, in milliseconds. This is
    // the throttle that keeps the display off the RF thread's back: at
    // the default 1.92 MS/s the receiver produces ~1875 windows of 1024
    // samples every second and we deliberately transform only a few
    // dozen of them (proportionally more at a higher rfSampleRateHz).
    // See SpectrumStage.h.
    int refreshIntervalMs = 50;

    // Weight given to the newest measurement when averaging frames
    // together, in (0, 1]. 1.0 disables averaging entirely (every frame
    // stands alone, which looks lively but noisy); lower values trade
    // responsiveness for a visibly steadier noise floor. Averaging
    // happens in linear power, not dB -- see SpectrumStage.cpp.
    float smoothingFactor = 0.4f;

    // Bottom and top of the displayed dB scale, in dBFS (0 dBFS = a
    // full-scale complex tone landing entirely in one bin). Everything
    // outside is clamped by the renderer.
    float minDb = -90.0f;
    float maxDb = 0.0f;

    // Furthest the display is allowed to zoom in, as a decimation
    // factor (a power of two). Bounded because one FFT window needs
    // this many times more input samples to fill, so deep zoom makes
    // the display update slowly: at 256x a single frame needs about a
    // seventh of a second of signal, and at 4096x it would need over
    // two seconds. 256 keeps the deepest zoom usable -- a 7.5 kHz span
    // at roughly 11 Hz resolution.
    int maxDecimation = 256;
};

// Which SpectrumSink Toaster should build.
enum class SpectrumRendererType {
    Bars,       // level-vs-frequency trace: precise amplitude, no history
    Waterfall,  // frequency-vs-time, level as colour: history, coarse amplitude
};

// Which AudioSink Toaster should build.
enum class AudioOutputMode {
    RawPcmStdout,  // headerless 16-bit PCM piped to stdout (e.g. into aplay/sox)
    WavFile,       // standard .wav file written to outputWavPath
};

// Which Filter implementation Toaster should build for anti-aliasing.
// Both are low-pass (that's what's correct before decimation) — this
// only chooses FIR vs IIR, not the filter's shape.
enum class FilterImplementation {
    Fir,  // FIR low-pass (linear phase, more CPU, tunable steepness via taps)
    Iir,  // IIR low-pass (2nd-order biquad, cheaper, slight phase distortion)
};

// Configuration for the IQ-to-classification feature-extraction chain.
// Used by ClassificationStage. Like SpectrumStageConfig, it always
// looks at the whole captured band (rfSampleRateHz) rather than
// whatever slice a user has zoomed the spectrum display to — see
// classification/ClassificationStage.h for why.
struct ClassificationStageConfig {
    double centerFreqHz = 0.0;          // updated live as "tune <hz>" commands arrive
    uint32_t rfSampleRateHz = 1920000;

    // FFT window size. Independent of the spectrum-visual stage's -n
    // (they can legitimately want different resolution/refresh
    // tradeoffs), and, like Fft everywhere else, rounded up to a power
    // of two if it isn't one already.
    int fftSize = 1024;

    // How often a window is analyzed, in milliseconds. Slower than
    // spectrum-visual's default (50ms): this stage is about
    // characterizing what's present over time, not smooth animation, so
    // there's no benefit to matching a display's refresh rate, and a
    // slower rate means less RF-thread time spent here.
    int refreshIntervalMs = 200;

    // How far above the estimated noise floor (in dB) a bin must be to
    // count as part of a signal, and how many consecutive such bins are
    // needed before that's reported as a detection rather than
    // dismissed as a noise spike. Raise detectionMarginDb or minRunBins
    // if a noisy RF environment is producing spurious detections; lower
    // them to catch weaker/narrower signals at the cost of more false
    // positives.
    float detectionMarginDb = 12.0f;
    int minRunBins = 3;

    // Where detections are written; "-" means stderr. Defaults to a
    // file rather than stderr because stderr is also where the
    // spectrum-visual stage's live-redrawing renderer lives — see
    // classification/TerminalClassificationSink.h.
    std::string logPath = "classification.log";
};

// Configuration for ScannerStage's sweep. Used only when the "scanner"
// stage is selected. See scanner/ScannerStage.h for the sweep's state
// machine (Settling -> Measuring -> either back to Settling at the next
// frequency, or Locked -> Settling once the lock expires).
struct ScannerStageConfig {
    double startFreqHz = 88000000.0;
    double endFreqHz = 108000000.0;

    // The initial survey's step size -- deliberately coarser than what
    // you'd want to actually sit and listen on, so a wide range sweeps
    // quickly. See ScannerStage.h's "Coarse sweep, then fine
    // refinement" section for what happens once a step this size finds
    // something.
    double coarseStepHz = 500000.0;

    // Once a coarse step measures above thresholdDb, the sub-sweep
    // around it uses this step size instead, across fineRangeHz
    // centered on the coarse hit -- narrow enough to actually localize
    // the signal's peak rather than lock wherever the coarse grid
    // happened to land. Set either fineStepHz or fineRangeHz to 0 to
    // disable refinement entirely and lock exactly on the coarse hit.
    double fineStepHz = 25000.0;

    // How wide a neighborhood around a coarse hit gets refined. Default
    // matches coarseStepHz's default, which is the right size to fully
    // cover the gap between adjacent coarse points -- if you raise
    // coarseStepHz, consider raising this to match; if it's much
    // smaller than coarseStepHz, refinement might not reach the true
    // peak if the coarse hit landed well off it.
    double fineRangeHz = 500000.0;

    // Discarded after each retune, before measuring -- covers the
    // tuner's PLL settling time. How long this actually needs to be
    // varies by dongle; if hits seem to happen on the wrong frequency,
    // or measurements look inconsistent, try raising it. Note that the
    // actual USB retune happens on Receiver's dedicated tuner thread
    // (see rf/Receiver.h's "Retuning" section), not synchronously the
    // instant this stage asks for it, so this timer's clock starts
    // very slightly before the real retune does -- a difference of a
    // scheduling quantum, not something the default here needs to
    // account for, but worth knowing if squeezing settleMs very low.
    int settleMs = 15;

    // How long RSSI is accumulated before deciding whether a channel is
    // busy. Longer is a steadier (less noise-sensitive) reading at the
    // cost of a slower sweep -- and, during fine refinement, this cost
    // is paid once per fine candidate, so raising it trades off against
    // how many candidates fineRangeHz/fineStepHz produce.
    int measureMs = 20;

    // Absolute RSSI level, in dBFS, at or above which a channel is
    // considered busy. There's no noise-floor-relative option here
    // (unlike ClassificationStage's detectionMarginDb) -- RSSI depends
    // heavily on the tuner's current gain setting, so this needs
    // tuning to match whatever gain the receiver is using (see -g in
    // main.cpp) rather than being a fixed, portable default.
    float thresholdDb = -30.0f;

    // How long to stay tuned to a hit (letting audio/classification
    // have it) before resuming the sweep, in milliseconds. Fixed
    // duration regardless of whether the signal is still present when
    // it elapses -- a future improvement could re-check and extend or
    // cut the lock short instead.
    //
    // This is only ScannerStage's own knob -- it always just holds for
    // whatever lockMs it's given. The *decision* of which value that
    // is (this one, meant for actually listening, or
    // quickConfirmMs below, meant for a scan running without audio) is
    // made by Toaster before ScannerStage is even constructed -- see
    // Toaster.cpp's comment on why that's the right place for it.
    int lockMs = 8000;

    // How long to hold a hit when nothing is listening to it live (the
    // "audio" stage isn't selected). Long enough for
    // ClassificationStage's default refresh interval to complete at
    // least one window (so a scan-only run still gets every hit
    // classified), far shorter than lockMs, which is sized for a human
    // to actually hear something. See Toaster.cpp for exactly when this
    // applies instead of lockMs.
    int quickConfirmMs = 300;

    // Where lock/resume events are written; "-" means stderr. Same
    // reasoning as ClassificationStageConfig::logPath.
    std::string logPath = "scan.log";
};

// Full configuration for a Toaster: everything needed to receive,
// process, and output a signal, in one place. Toaster splits this into
// a ReceiverConfig and an AudioStageConfig, and builds the Filter and
// AudioSink described here.
struct ToasterConfig {
    // Receiving
    double centerFreqHz = 0.0;
    int deviceIndex = 0;
    int manualGainTenthsDb = -1;  // -1 means auto gain
    // How much RF bandwidth is captured; also the widest span the
    // spectrum display can ever zoom out to (see SpectrumStageConfig
    // above). Settable via -r.
    uint32_t rfSampleRateHz = 1920000;

    // Audio pipeline (DSP)
    uint32_t audioSampleRateHz = 48000;
    double deemphasisMicroseconds = 75.0;
    int audioPostTuneMuteMs = 20;
    int audioPostTuneFadeInMs = 15;

    // Anti-aliasing filter (applied to the demodulated signal, before
    // decimation)
    FilterImplementation antiAliasFilterType = FilterImplementation::Fir;
    int firNumTaps = 63;              // used when antiAliasFilterType == Fir
    double audioCutoffHz = 15000.0;   // used by both Fir and Iir

    // Spectrum display (used only when the "spectrum-visual" stage is
    // selected; see spectrum/SpectrumStage.h)
    SpectrumRendererType spectrumRenderer = SpectrumRendererType::Bars;
    int spectrumFftSize = 1024;
    int spectrumRefreshIntervalMs = 50;
    float spectrumSmoothingFactor = 0.4f;
    float spectrumMinDb = -90.0f;
    float spectrumMaxDb = 0.0f;
    int spectrumMaxDecimation = 256;

    // Classification (used only when the "classification" stage is
    // selected; see classification/ClassificationStage.h)
    int classificationFftSize = 1024;
    int classificationRefreshIntervalMs = 200;
    float classificationDetectionMarginDb = 12.0f;
    int classificationMinRunBins = 3;
    std::string classificationLogPath = "classification.log";

    // Scanner (used only when the "scanner" stage is selected; see
    // scanner/ScannerStage.h)
    double scanStartFreqHz = 88000000.0;
    double scanEndFreqHz = 108000000.0;
    double scanCoarseStepHz = 500000.0;
    double scanFineStepHz = 25000.0;
    double scanFineRangeHz = 500000.0;
    int scanSettleMs = 15;
    int scanMeasureMs = 20;
    float scanThresholdDb = -30.0f;
    int scanLockMs = 8000;
    // True once --scan-lock-ms is explicitly passed -- distinguishes
    // "the user wants exactly this" from "this is just the default",
    // which matters because the effective lock duration used depends on
    // whether "audio" is also selected (see Toaster.cpp); an explicit
    // value always wins over that.
    bool scanLockMsExplicit = false;
    int scanQuickConfirmMs = 300;
    std::string scanLogPath = "scan.log";

    // Output
    AudioOutputMode outputMode = AudioOutputMode::RawPcmStdout;
    std::string outputWavPath;      // used when outputMode == WavFile
    float outputGain = 12000.0f;    // tune if audio clips or is too quiet

    // Which pipeline stages to run, by key (see StageRegistry.h and
    // Toaster::allStageDescriptors()), e.g. {"audio"} or
    // {"spectrum-visual", "spectrum-electronic"}. Empty means "ask
    // interactively on the console at startup" -- see
    // core/StageSelection.h. Stages are independently optional: nothing
    // forces two of them to run together, which matters once some are
    // heavy enough (a live spectrum display) that a user may only want
    // one running at a time.
    std::vector<std::string> requestedStageKeys;
};
