#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

// Analysis window coefficients for the FFT.
//
// Why a window is needed at all: an FFT implicitly assumes the block of
// samples it's given repeats forever. A real signal almost never lines
// up with the ends of the block, so that implied repetition has a
// discontinuity at the seam, and a discontinuity is broadband -- it
// smears energy from a strong carrier across every bin ("spectral
// leakage"), which on a spectrum display looks like a noise floor tens
// of dB higher than the receiver's actual one. Tapering the block to
// zero at both ends removes the seam.
//
// Hann specifically: its first sidelobe is about -31 dB with sidelobes
// falling off at 18 dB/octave, which is a good match for looking at
// broadcast FM (where neighbouring stations are strong and close). The
// cost is a main lobe twice as wide as no window at all, i.e. slightly
// worse ability to resolve two signals a bin or two apart -- an
// excellent trade when the alternative is a leakage floor that buries
// weak signals entirely.
//
// Header-only, like Decimator and FmDemodulator: it's a handful of
// lines with no state to manage.
namespace window {

// Periodic (not symmetric) Hann window of `length` points. The periodic
// form divides by N rather than N-1, which is the correct choice for
// spectral analysis -- it makes the window seamless under the FFT's
// assumed periodic extension, where the symmetric form (the right
// choice for FIR filter design, which is why FirDesign.h uses Hamming
// that way) would repeat its endpoint.
inline std::vector<float> makeHann(size_t length) {
    std::vector<float> coefficients(length);
    if (length == 0) return coefficients;
    if (length == 1) {
        coefficients[0] = 1.0f;
        return coefficients;
    }
    constexpr double kTwoPi = 6.283185307179586476925286766559;
    for (size_t n = 0; n < length; ++n) {
        const double phase = kTwoPi * static_cast<double>(n) / static_cast<double>(length);
        coefficients[n] = static_cast<float>(0.5 * (1.0 - std::cos(phase)));
    }
    return coefficients;
}

// Sum of the window's coefficients, i.e. its coherent gain times its
// length. Dividing an FFT bin's magnitude by this is what makes a
// full-scale complex tone read as 0 dBFS regardless of which window
// (or what window length) is in use -- without it, changing the FFT
// size would slide the whole display up or down.
inline double coherentGainSum(const std::vector<float>& coefficients) {
    double sum = 0.0;
    for (float coefficient : coefficients) sum += coefficient;
    return sum > 0.0 ? sum : 1.0;
}

}  // namespace window
