#include "dsp/DigitalDownConverter.h"

#include <algorithm>
#include <cmath>

#include "dsp/filters/FirDesign.h"

namespace {

constexpr double kTwoPi = 6.283185307179586476925286766559;

// Taps per decimate-by-2 stage. Enough for roughly 50 dB of stopband
// rejection with a Hamming window at the cutoff below, which keeps
// aliased images far enough down to not be mistaken for signals.
constexpr int kStageTaps = 63;

// Cutoff as a fraction of each stage's own input rate. Halving the rate
// puts the new Nyquist at 0.25, so the passband has to end below that
// and the stopband begin above it. Ending at 0.225 leaves a transition
// band of 0.05 for the filter to roll off in -- narrow enough to keep
// nearly all of the decimated span usable, wide enough that 63 taps can
// do it. The cost is that the outermost ~10% of a zoomed span has
// weaker alias rejection than the middle.
constexpr double kStageCutoffFraction = 0.225;

// The mixer's phasor drifts off the unit circle as rounding errors
// accumulate in repeated multiplication. Rescaling periodically costs
// one square root every few thousand samples and bounds the error,
// where letting it run unchecked would slowly change the amplitude of
// everything measured.
constexpr size_t kRenormalizeInterval = 4096;

int roundUpToPowerOfTwo(int value) {
    int result = 1;
    while (result < value) result <<= 1;
    return result;
}

}  // namespace

DigitalDownConverter::DigitalDownConverter(double inputSampleRateHz)
    : inputSampleRateHz_(inputSampleRateHz) {
    // Designed once against a normalized rate of 1.0: every stage sees
    // the same cutoff relative to its own input rate, so one set of
    // coefficients serves the whole cascade however deep it gets.
    stageCoefficients_ = FirDesign::lowPass(kStageTaps, kStageCutoffFraction, 1.0);
    configure(0.0, 1);
}

DigitalDownConverter::HalfRateStage::HalfRateStage(const std::vector<float>* coefficients)
    : coefficients_(coefficients) {
    history_.assign(coefficients_->size() * 2, std::complex<float>(0.0f, 0.0f));
}

void DigitalDownConverter::HalfRateStage::reset() {
    std::fill(history_.begin(), history_.end(), std::complex<float>(0.0f, 0.0f));
    position_ = 0;
    phase_ = 0;
}

bool DigitalDownConverter::HalfRateStage::push(const std::complex<float>& input,
                                                std::complex<float>& out) {
    const int taps = static_cast<int>(coefficients_->size());

    history_[static_cast<size_t>(position_)] = input;
    history_[static_cast<size_t>(position_ + taps)] = input;
    position_ = (position_ + 1) % taps;

    // Only convolve on the samples that survive decimation. Computing
    // the ones about to be discarded would double this stage's cost for
    // no benefit whatsoever.
    phase_ ^= 1;
    if (phase_ != 0) return false;

    const std::complex<float>* window = history_.data() + position_;
    const float* coefficients = coefficients_->data();
    float realSum = 0.0f;
    float imagSum = 0.0f;
    for (int tap = 0; tap < taps; ++tap) {
        realSum += window[tap].real() * coefficients[tap];
        imagSum += window[tap].imag() * coefficients[tap];
    }
    out = std::complex<float>(realSum, imagSum);
    return true;
}

void DigitalDownConverter::configure(double offsetHz, int decimation) {
    decimation_ = roundUpToPowerOfTwo(std::max(1, decimation));
    offsetHz_ = offsetHz;

    int stageCount = 0;
    for (int factor = decimation_; factor > 1; factor >>= 1) ++stageCount;

    stages_.clear();
    stages_.reserve(static_cast<size_t>(stageCount));
    for (int stage = 0; stage < stageCount; ++stage) {
        stages_.emplace_back(&stageCoefficients_);
    }

    // Mixing by -offset brings the slice we want down to DC, which is
    // where the FFT's centre bin is.
    const double radiansPerSample = -kTwoPi * offsetHz_ / inputSampleRateHz_;
    mixerStep_ = std::complex<float>(static_cast<float>(std::cos(radiansPerSample)),
                                      static_cast<float>(std::sin(radiansPerSample)));
    reset();
}

size_t DigitalDownConverter::warmUpSamples() const {
    // Each stage needs its history filled before its output means
    // anything; expressed in *output* samples, that's half a filter
    // length per stage, and the stages are in series. Rounded up
    // generously -- these samples are cheap and a transient leaking
    // into the FFT is not.
    return static_cast<size_t>(kStageTaps) * (stages_.size() + 1);
}

void DigitalDownConverter::reset() {
    for (HalfRateStage& stage : stages_) stage.reset();
    // Phase restarts at zero every window. The absolute phase of the
    // mixer has no effect on a magnitude spectrum, so there's nothing
    // to preserve across windows and plenty to lose (drift) by trying.
    mixerPhase_ = std::complex<float>(1.0f, 0.0f);
    samplesSinceRenormalize_ = 0;
}

void DigitalDownConverter::process(const std::complex<float>* input, size_t count,
                                    std::vector<std::complex<float>>& out) {
    for (size_t n = 0; n < count; ++n) {
        std::complex<float> sample = input[n];

        if (offsetHz_ != 0.0) {
            sample *= mixerPhase_;
            mixerPhase_ *= mixerStep_;
            if (++samplesSinceRenormalize_ >= kRenormalizeInterval) {
                const float magnitude = std::abs(mixerPhase_);
                if (magnitude > 0.0f) mixerPhase_ /= magnitude;
                samplesSinceRenormalize_ = 0;
            }
        }

        // Walk the cascade. A sample that a stage decides to discard
        // simply stops there; only samples surviving every stage reach
        // the output.
        bool alive = true;
        for (HalfRateStage& stage : stages_) {
            std::complex<float> decimated;
            if (!stage.push(sample, decimated)) {
                alive = false;
                break;
            }
            sample = decimated;
        }
        if (alive) out.push_back(sample);
    }
}
