#pragma once

#include <complex>
#include <cstddef>
#include <vector>

// In-place radix-2 Cooley-Tukey FFT over std::complex<float>.
//
// Written by hand rather than pulling in FFTW/KissFFT/pocketfft because
// this project's only external dependency is librtlsdr, and a spectrum
// display at a few dozen frames per second needs nothing like the
// throughput those libraries exist to provide -- one 1024-point
// transform is on the order of ten microseconds here, and we do roughly
// twenty of them a second (see spectrum/SpectrumStage.h for why the
// other ~1800 windows per second are dropped rather than transformed).
// If a future stage ever needs a transform on every window at the full
// sample rate, swapping this class out for a real FFT library is a
// self-contained change: nothing outside Fft.cpp knows how the
// transform is computed.
//
// The size is fixed at construction so the twiddle factors and the
// bit-reversal permutation are computed exactly once, rather than on
// every transform. That's the whole reason this is a class holding
// state instead of a free function.
//
// Not a template, for the same reason the Filter hierarchy isn't (see
// README.md): every sample in this project is float, so genericity over
// the sample type would buy nothing.
class Fft {
public:
    // `size` must be a power of two. A non-power-of-two is rounded *up*
    // to the next one rather than rejected, so an Fft can never exist in
    // an unusable state; call size() if you need to know what you
    // actually got. (main.cpp validates the -n flag itself, so this
    // rounding is a defensive fallback, not the primary check.)
    explicit Fft(size_t size);

    // Forward transform, in place. `data` must point at exactly size()
    // elements. Output is in the usual FFT bin order: bin k holds
    // frequency k * sampleRate / size for k < size/2, and
    // (k - size) * sampleRate / size for k >= size/2 (i.e. the negative
    // frequencies live in the upper half). Callers wanting a
    // left-to-right frequency axis need to shift it -- SpectrumStage
    // does this while it reads the bins out, which costs nothing extra
    // since it's already walking them.
    void transform(std::complex<float>* data) const;

    size_t size() const { return size_; }

    // Smallest power of two >= value (minimum 1). Exposed because
    // callers validating user input want the same notion of "valid
    // size" this class uses.
    static size_t roundUpToPowerOfTwo(size_t value);

private:
    size_t size_;

    // bitReversedIndex_[i] is where element i moves to in the
    // permutation that precedes the butterfly passes.
    std::vector<size_t> bitReversedIndex_;

    // twiddles_[m] = exp(-2*pi*i * m / size), for m in [0, size/2).
    // Only the first half is needed: every twiddle any butterfly pass
    // asks for is one of these.
    std::vector<std::complex<float>> twiddles_;
};
