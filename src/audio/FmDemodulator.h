#pragma once

#include <complex>

// Quadrature FM demodulator.
//
// Converts a stream of complex IQ samples into a stream of instantaneous
// frequency estimates (in radians), one output sample per input sample.
//
// Uses the "delta phase" method: angle(x[n] * conj(x[n-1])) instead of
// differentiating atan2(x[n]) directly, which avoids glitches when the
// raw phase wraps across +/-pi. Expressed directly via std::complex<float>
// multiplication/conjugation/arg rather than manual real/imaginary
// arithmetic -- identical math, easier to read, and matches what the
// rest of the pipeline uses for IQ samples now that Receiver produces
// std::complex<float> directly.
class FmDemodulator {
public:
    FmDemodulator() = default;

    float demodulate(std::complex<float> sample) {
        std::complex<float> delta = sample * std::conj(prev_);
        prev_ = sample;
        return std::arg(delta);
    }

    void reset() { prev_ = std::complex<float>(0.0f, 0.0f); }

private:
    std::complex<float> prev_{0.0f, 0.0f};
};
