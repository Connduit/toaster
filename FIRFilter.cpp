
#include "FIRFilter.h"

#include <cmath>
#include <stdexcept>

FIRFilter::FIRFilter(
    float sampleRate,
    float cutoffFrequency,
    int numberOfTaps)
    : 
    sampleRate_(sampleRate),
    cutoffFrequency_(cutoffFrequency),
    numberOfTaps_(numberOfTaps),
    coefficients_(numberOfTaps),
    buffer_(numberOfTaps, 0.0f),
    bufferIndex_(0)
{
    if (sampleRate_ <= 0.0f)
        throw std::invalid_argument("Invalid sample rate");

    if (cutoffFrequency_ <= 0.0f ||
        cutoffFrequency_ >= sampleRate_ / 2.0f)
        throw std::invalid_argument("Invalid cutoff frequency");

    if (numberOfTaps_ <= 0 ||
        numberOfTaps_ % 2 == 0)
        throw std::invalid_argument(
            "Number of taps must be positive and odd");

    designLowPass();
}

void FIRFilter::designLowPass()
{
    constexpr float PI = 3.14159265358979323846f;

    const int M = numberOfTaps_ - 1;

    for (int n = 0; n < numberOfTaps_; ++n)
    {
        const float k = static_cast<float>(n - M / 2);

        float sinc;

        if (k == 0.0f)
        {
            sinc = 2.0f * cutoffFrequency_ / sampleRate_;
        }
        else
        {
            sinc = std::sin( 2.0f * PI * cutoffFrequency_ * k / sampleRate_) / (PI * k);
        }

        // Hamming window
        const float window = 0.54f - 0.46f * std::cos( 2.0f * PI * n / M);
        coefficients_[n] = sinc * window;
    }

    // Normalize DC gain to 1
    float sum = 0.0f;

    for (float coefficient : coefficients_)
    {
        sum += coefficient;
    }

    for (float &coefficient : coefficients_)
    {
        coefficient /= sum;
    }
}

float FIRFilter::process(const float& input)
{
    buffer_[bufferIndex_] = input;

    float output = 0.0f;

    int index = bufferIndex_;

    for (int i = 0; i < numberOfTaps_; ++i)
    {
        output +=
            coefficients_[i] *
            buffer_[index];

        --index;

        if (index < 0)
            index = numberOfTaps_ - 1;
    }

    ++bufferIndex_;

    if (bufferIndex_ >= numberOfTaps_)
        bufferIndex_ = 0;

    return output;
}

//FIRFilter::FIRFilter(const std::vector<float>& coefficients)
//    : 
//    coefficients_(coefficients),
//    buffer_(coefficients.size(), 0.0f),
//    writeIndex_(0)
//{
//}
//
//float FIRFilter::process(float input)
//{
//    // Insert the newest sample into the circular buffer.
//    buffer_[writeIndex_] = input;
//
//    float output = 0.0f;
//
//    // Start at the newest sample.
//    std::size_t bufferIndex = writeIndex_;
//
//    for (std::size_t i = 0; i < coefficients_.size(); ++i)
//    {
//        output += coefficients_[i] * buffer_[bufferIndex];
//
//        // Move backward through the circular buffer.
//        if (bufferIndex == 0)
//        {
//            bufferIndex = buffer_.size() - 1;
//        }
//        else
//        {
//            --bufferIndex;
//        }
//    }
//
//    // Move to the next position in the circular buffer.
//    ++writeIndex_;
//
//    if (writeIndex_ >= buffer_.size())
//    {
//        writeIndex_ = 0;
//    }
//
//    return output;
//}

// FIRFilter::FIRFilter(
//     float sampleRate,
//     float cutoffFrequency,
//     int numTaps
// )
//     : delayIndex_(0),
//       sampleRate_(sampleRate),
//       cutoffFrequency_(cutoffFrequency),
//       numTaps_(numTaps)
// {
//     if (numTaps_ <= 0 || numTaps_ % 2 == 0)
//         throw std::invalid_argument(
//             "FIR filter requires a positive odd number of taps"
//         );
// 
//     if (cutoffFrequency_ <= 0.0f ||
//         cutoffFrequency_ >= sampleRate_ / 2.0f)
//     {
//         throw std::invalid_argument(
//             "Invalid filter cutoff frequency"
//         );
//     }
// 
//     coefficients_.resize(numTaps_);
// 
//     //iqDelayLine_.resize(numTaps_, std::complex<float>(0.0f, 0.0f));
//     iDelayLine_.resize(numTaps_, 0.0f);
//     qDelayLine_.resize(numTaps_, 0.0f);
//     audioDelayLine_.resize(numTaps_, 0.0f);
// 
//     designLowPass();
// }
// 
// void FIRFilter::designLowPass()
// {
//     const int center = numTaps_ / 2;
// 
//     float coefficientSum = 0.0f;
// 
//     for (int n = 0; n < numTaps_; ++n)
//     {
//         const int k = n - center;
// 
//         float sinc;
// 
//         if (k == 0)
//         {
//             sinc = 2.0f * cutoffFrequency_ / sampleRate_;
//         }
//         else
//         {
//             const float x =
//                 2.0f * static_cast<float>(M_PI) *
//                 cutoffFrequency_ * static_cast<float>(k) /
//                 sampleRate_;
// 
//             sinc = std::sin(x) /
//                    (static_cast<float>(M_PI) * static_cast<float>(k));
//         }
// 
//         // Hamming window
//         const float window =
//             0.54f -
//             0.46f *
//             std::cos(
//                 2.0f * static_cast<float>(M_PI) *
//                 static_cast<float>(n) /
//                 static_cast<float>(numTaps_ - 1)
//             );
// 
//         coefficients_[n] = sinc * window;
//         coefficientSum += coefficients_[n];
//     }
// 
//     // Normalize DC gain to 1.
//     for (float& coefficient : coefficients_)
//         coefficient /= coefficientSum;
// }
// 
// // TODO: not efficient enough
// IQData FIRFilter::process(const IQData& input)
// {
//     IQData output;
//     //output.reserve(input.size());
//     output.resize(input.size());
// 
//     //for (const auto& sample : input)
//     for (size_t i = 0; i < input.size(); ++i)
//     {
//         // Store the current IQ sample.
//         //iqDelayLine_[delayIndex_] = sample;
//         iDelayLine_[delayIndex_] = input[i].real();
//         qDelayLine_[delayIndex_] = input[i].imag();
// 
//         std::complex<float> filtered(0.0f, 0.0f);
//         float filteredI = 0.0f;
//         float filteredQ = 0.0f;
// 
//         // FIR convolution.
//         int index = delayIndex_;
// 
//         for (int tap = 0; tap < numTaps_; ++tap)
//         {
//             //filtered += iqDelayLine_[index] * coefficients_[tap];
//             filteredI += iDelayLine_[index] * coefficients_[tap];
//             filteredQ += qDelayLine_[index] * coefficients_[tap];
// 
//             --index;
// 
//             if (index < 0)
//                 index = numTaps_ - 1;
//         }
// 
//         //output.push_back(filtered);
//         output[i] = std::complex<float>(filteredI, filteredQ);
// 
//         // Advance circular buffer.
//         ++delayIndex_;
// 
//         if (delayIndex_ >= numTaps_)
//             delayIndex_ = 0;
//     }
// 
//     return output;
// }
// 
// AudioData FIRFilter::process(const AudioData& input)
// {
//     AudioData output;
//     //output.reserve(input.size());
//     output.resize(input.size());
// 
//     for (const float sample : input)
//     {
//         audioDelayLine_[delayIndex_] = sample;
// 
//         float filtered = 0.0f;
// 
//         int index = delayIndex_;
// 
//         for (int tap = 0; tap < numTaps_; ++tap)
//         {
//             filtered +=
//                 audioDelayLine_[index] *
//                 coefficients_[tap];
// 
//             --index;
// 
//             if (index < 0)
//                 index = numTaps_ - 1;
//         }
// 
//         output.push_back(filtered);
// 
//         ++delayIndex_;
// 
//         if (delayIndex_ >= numTaps_)
//             delayIndex_ = 0;
//     }
// 
//     return output;
// }
// 