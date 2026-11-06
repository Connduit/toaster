#pragma once

#include <complex>
#include <cstddef>
#include <vector>

// Selects a narrow slice of a wide IQ stream and hands it back at a
// proportionally lower sample rate: mix the slice down to DC, low-pass
// filter, decimate. This is the standard "digital downconverter", and
// it's what makes zooming a spectrum display mean something.
//
// ## Why this exists rather than just showing fewer FFT bins
//
// Cropping the FFT output to the middle 10% of its bins would also fill
// the screen with a tenth of the span, but it would not show anything
// new: those are the same bins, the same ~2.8 kHz apart, simply drawn
// wider. Zoom in far enough and you get a handful of enormous blocks.
//
// Decimating first is different. Running a 1024-point FFT on a stream
// that's been decimated by 64 covers 1/64th of the bandwidth with the
// same 1024 bins, so each bin is 64x narrower and the display resolves
// detail that genuinely was not visible before -- a 2.8 kHz bin becomes
// a 44 Hz one. That is what a real analyser does when you reduce span,
// and why its resolution bandwidth readout drops as you zoom.
//
// The cost is time: one FFT window now needs `decimation` times as many
// input samples to fill, so at high zoom the display updates more
// slowly. That's not an implementation artifact, it's the uncertainty
// principle showing up in a user interface -- resolving finer detail in
// frequency requires observing for longer. Real analysers behave the
// same way.
//
// ## Structure
//
// Decimation is done as a cascade of decimate-by-2 stages rather than
// one big filter, which is why the factor must be a power of two. A
// single filter decimating by 64 in one step would need a transition
// band 64x narrower and therefore hundreds of times more taps, and all
// of them would run at the full input rate. In a cascade, each stage
// faces the same *relative* transition width, so every stage can use
// the same modest filter, and each stage after the first runs at half
// the rate of the one before -- the total work converges to roughly
// twice the first stage's, no matter how deep the cascade goes.
//
// Each stage computes output samples only on the phases it's going to
// keep, instead of filtering everything and then throwing half away.
// That halves the work again, and is the reason this doesn't simply
// reuse FirLowPassFilter + Decimator: Filter::process() is obliged to
// produce an output for every input, which is exactly the work a
// decimating filter exists to avoid.
class DigitalDownConverter {
public:
    explicit DigitalDownConverter(double inputSampleRateHz);

    // Selects the slice centred `offsetHz` away from the input stream's
    // centre, at 1/`decimation` of the input rate. `decimation` must be
    // a power of two; it's rounded up to one if it isn't. Rebuilds the
    // cascade, so call it when the view changes, not per sample.
    void configure(double offsetHz, int decimation);

    double offsetHz() const { return offsetHz_; }
    int decimation() const { return decimation_; }
    double outputSampleRateHz() const { return inputSampleRateHz_ / decimation_; }

    // True when no mixing or decimating is needed, i.e. the caller can
    // skip this class entirely and use its input unchanged.
    bool isPassThrough() const { return decimation_ == 1 && offsetHz_ == 0.0; }

    // How many output samples should be discarded after a reset()
    // before the rest are trustworthy. Filters start with an empty
    // history, so their first outputs reflect that emptiness rather
    // than the signal; this is the cascade's settling time.
    size_t warmUpSamples() const;

    // Clears filter history and restarts the mixer's phase. Called at
    // the start of each window, because this stage deliberately skips
    // most of the incoming samples (see SpectrumStage) -- so the filter
    // history from the last window is not continuous with this one's
    // input and would otherwise inject a transient.
    void reset();

    // Pushes `count` input samples through, appending whatever output
    // samples emerge to `out`. Typically far fewer than `count`: one
    // per `decimation` inputs, give or take the cascade's phase.
    void process(const std::complex<float>* input, size_t count,
                  std::vector<std::complex<float>>& out);

private:
    // One decimate-by-2 complex FIR. Real coefficients applied to a
    // complex signal, which is just the same filter run on I and Q.
    class HalfRateStage {
    public:
        explicit HalfRateStage(const std::vector<float>* coefficients);
        void reset();
        // Returns true and writes `out` on the samples it keeps.
        bool push(const std::complex<float>& input, std::complex<float>& out);

    private:
        const std::vector<float>* coefficients_;
        // Double-length, every sample written to both halves, so a
        // contiguous slice is always a correctly-ordered window with no
        // wraparound in the convolution loop. Same trick FirFilter uses,
        // and for the same reason.
        std::vector<std::complex<float>> history_;
        int position_ = 0;
        int phase_ = 0;
    };

    double inputSampleRateHz_;
    double offsetHz_ = 0.0;
    int decimation_ = 1;

    // Shared by every stage: each one sees the same normalized cutoff
    // relative to its own input rate, so they all want the same taps.
    std::vector<float> stageCoefficients_;
    std::vector<HalfRateStage> stages_;

    // Numerically-controlled oscillator, advanced by repeated complex
    // multiplication rather than calling sin/cos per sample.
    std::complex<float> mixerPhase_{1.0f, 0.0f};
    std::complex<float> mixerStep_{1.0f, 0.0f};
    size_t samplesSinceRenormalize_ = 0;
};
