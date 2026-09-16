#ifndef TOASTER_FIR_FILTER_H
#define TOASTER_FIR_FILTER_H

#include "Filter.h"

#include <cstddef>
#include <vector>

class FIRFilter : public Filter
{
public:
    //FIRFilter( float sampleRate, float cutoffFrequency, int numberOfTaps);

    float process(float sample) override;
    void reset() override;

protected:
    FIRFilter() = default;

    // Subclasses call this once, from their constructor, after
    // designing their coefficients.
    void setCoefficients(std::vector<float> coefficients);

private:
    std::vector<float> coeffs_;
    std::vector<float> history_; // buffer_ ?
    int historyPos_ = 0; // bufferIndex_

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