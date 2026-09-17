#ifndef TOASTER_FIR_FILTER_H
#define TOASTER_FIR_FILTER_H

#include <vector>

#include "Filter.h"

// Base class for FIR (finite impulse response) filters. Implements the
// convolution every FIR filter needs, regardless of what shape (low-pass,
// high-pass, band-pass, band-stop) its coefficients describe. Concrete
// subclasses just design coefficients (see FirDesign.h) and hand them to
// setCoefficients() from their constructor — they never touch the buffer
// or convolution logic here.
//
// The history buffer is double-length (2 * numTaps) and every new sample
// is written to both halves. That mirroring means a contiguous slice of
// numTaps elements starting at historyPos_ always holds the correct,
// correctly-ordered window — no wraparound handling needed in the hot
// convolution loop, and no modulo operation per tap. (An earlier version
// computed history_[idx % size] once per tap, which cost roughly N
// modulo operations per sample for an N-tap filter — a real hotspot at
// the full pre-decimation sample rate this filter runs at, since integer
// modulo is slow and doesn't vectorize.) Advancing historyPos_ each
// sample still needs exactly one modulo, not N.
//
// Not meant to be instantiated directly: use one of the FirLowPassFilter
// / FirHighPassFilter / FirBandPassFilter / FirBandStopFilter subclasses.
class FIRFilter : public Filter {
public:
    float process(float sample) override;
    void reset() override;

protected:
    FIRFilter() = default;

    // Subclasses call this once, from their constructor, after
    // designing their coefficients.
    void setCoefficients(std::vector<float> coefficients);

private:
    std::vector<float> coeffs_;
    std::vector<float> history_;  // double-length; see class comment
    int numTaps_ = 0;
    int historyPos_ = 0;
};

//class FIRFilter : public Filter
//{
//public:
//    //FIRFilter(const std::vector<float>& coefficients);
//    FIRFilter(float sampleRate, float cutoffFrequency, int numberOfTaps);
//
//    float process(float input) override;
//private:
//    std::vector<float> coefficients_;
//    std::vector<float> buffer_;
//
//    std::size_t writeIndex_;
//};

// class FIRFilter : public Filter 
// {
// public:
// 
//     FIRFilter(float sampleRate, float cutoffFrequency, int numTaps);
// 
//     IQData process(const IQData& input);
//     AudioData process(const AudioData& input);
// 
// private:
//     void designLowPass();
//     std::vector<float> coefficients_;
// 
//     //std::vector<std::complex<float>> iqDelayLine_;
//     std::vector<float> iDelayLine_;
//     std::vector<float> qDelayLine_;
//     std::vector<float> audioDelayLine_;
// 
//     int delayIndex_;
// 
//     float sampleRate_;
//     float cutoffFrequency_;
//     int numTaps_;
// };

#endif