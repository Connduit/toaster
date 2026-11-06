#include "dsp/Fft.h"

#include <cmath>
#include <utility>

namespace {

constexpr double kTwoPi = 6.283185307179586476925286766559;

int bitCountFor(size_t powerOfTwoSize) {
    int bits = 0;
    while ((size_t{1} << bits) < powerOfTwoSize) ++bits;
    return bits;
}

size_t reverseBits(size_t value, int bitCount) {
    size_t reversed = 0;
    for (int i = 0; i < bitCount; ++i) {
        reversed = (reversed << 1) | (value & size_t{1});
        value >>= 1;
    }
    return reversed;
}

}  // namespace

size_t Fft::roundUpToPowerOfTwo(size_t value) {
    size_t result = 1;
    while (result < value) result <<= 1;
    return result;
}

Fft::Fft(size_t size) : size_(roundUpToPowerOfTwo(size == 0 ? 1 : size)) {
    const int bits = bitCountFor(size_);

    bitReversedIndex_.resize(size_);
    for (size_t i = 0; i < size_; ++i) {
        bitReversedIndex_[i] = reverseBits(i, bits);
    }

    twiddles_.resize(size_ / 2 == 0 ? 1 : size_ / 2);
    for (size_t m = 0; m < size_ / 2; ++m) {
        const double angle = -kTwoPi * static_cast<double>(m) / static_cast<double>(size_);
        twiddles_[m] = std::complex<float>(static_cast<float>(std::cos(angle)),
                                            static_cast<float>(std::sin(angle)));
    }
}

void Fft::transform(std::complex<float>* data) const {
    if (size_ < 2) return;

    // Permute into bit-reversed order first, so the butterfly passes
    // below can read and write strictly in place. Swapping only when
    // i < reversed avoids undoing every swap by performing it twice.
    for (size_t i = 0; i < size_; ++i) {
        const size_t reversed = bitReversedIndex_[i];
        if (i < reversed) std::swap(data[i], data[reversed]);
    }

    // Standard iterative Cooley-Tukey: log2(size) passes, each combining
    // pairs of half-length transforms into one of length `length`.
    for (size_t length = 2; length <= size_; length <<= 1) {
        const size_t half = length / 2;

        // Within a pass of length `length`, the j-th butterfly needs
        // exp(-2*pi*i * j / length), which is twiddles_[j * twiddleStep]
        // given twiddles_ is indexed against the full transform size.
        const size_t twiddleStep = size_ / length;

        for (size_t start = 0; start < size_; start += length) {
            for (size_t j = 0; j < half; ++j) {
                const std::complex<float> twiddle = twiddles_[j * twiddleStep];
                const std::complex<float> even = data[start + j];
                const std::complex<float> odd = twiddle * data[start + j + half];
                data[start + j] = even + odd;
                data[start + j + half] = even - odd;
            }
        }
    }
}
