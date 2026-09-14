#include "Filter.h"

#include <cmath>
#include <stdexcept>

Filter::Filter(
    float sampleRate,
    float cutoffFrequency,
    int numTaps
)
    : delayIndex_(0),
      sampleRate_(sampleRate),
      cutoffFrequency_(cutoffFrequency),
      numTaps_(numTaps)
{
    if (numTaps_ <= 0 || numTaps_ % 2 == 0)
        throw std::invalid_argument(
            "FIR filter requires a positive odd number of taps"
        );

    if (cutoffFrequency_ <= 0.0f ||
        cutoffFrequency_ >= sampleRate_ / 2.0f)
    {
        throw std::invalid_argument(
            "Invalid filter cutoff frequency"
        );
    }

    coefficients_.resize(numTaps_);

    //iqDelayLine_.resize(numTaps_, std::complex<float>(0.0f, 0.0f));
    iDelayLine_.resize(numTaps_, 0.0f);
    qDelayLine_.resize(numTaps_, 0.0f);
    audioDelayLine_.resize(numTaps_, 0.0f);

    designLowPass();
}

void Filter::designLowPass()
{
    const int center = numTaps_ / 2;

    float coefficientSum = 0.0f;

    for (int n = 0; n < numTaps_; ++n)
    {
        const int k = n - center;

        float sinc;

        if (k == 0)
        {
            sinc = 2.0f * cutoffFrequency_ / sampleRate_;
        }
        else
        {
            const float x =
                2.0f * static_cast<float>(M_PI) *
                cutoffFrequency_ * static_cast<float>(k) /
                sampleRate_;

            sinc = std::sin(x) /
                   (static_cast<float>(M_PI) * static_cast<float>(k));
        }

        // Hamming window
        const float window =
            0.54f -
            0.46f *
            std::cos(
                2.0f * static_cast<float>(M_PI) *
                static_cast<float>(n) /
                static_cast<float>(numTaps_ - 1)
            );

        coefficients_[n] = sinc * window;
        coefficientSum += coefficients_[n];
    }

    // Normalize DC gain to 1.
    for (float& coefficient : coefficients_)
        coefficient /= coefficientSum;
}

IQData Filter::process(const IQData& input)
{
    IQData output;
    //output.reserve(input.size());
    output.resize(input.size());

    //for (const auto& sample : input)
    for (size_t i = 0; i < input.size(); ++i)
    {
        // Store the current IQ sample.
        //iqDelayLine_[delayIndex_] = sample;
        iDelayLine_[delayIndex_] = input[i].real();
        qDelayLine_[delayIndex_] = input[i].imag();

        std::complex<float> filtered(0.0f, 0.0f);
        float filteredI = 0.0f;
        float filteredQ = 0.0f;

        // FIR convolution.
        int index = delayIndex_;

        for (int tap = 0; tap < numTaps_; ++tap)
        {
            //filtered += iqDelayLine_[index] * coefficients_[tap];
            filteredI += iDelayLine_[index] * coefficients_[tap];
            filteredQ += qDelayLine_[index] * coefficients_[tap];

            --index;

            if (index < 0)
                index = numTaps_ - 1;
        }

        //output.push_back(filtered);
        output[i] = std::complex<float>(filteredI, filteredQ);

        // Advance circular buffer.
        ++delayIndex_;

        if (delayIndex_ >= numTaps_)
            delayIndex_ = 0;
    }

    return output;
}

AudioData Filter::process(const AudioData& input)
{
    AudioData output;
    //output.reserve(input.size());
    output.resize(input.size());

    for (const float sample : input)
    {
        audioDelayLine_[delayIndex_] = sample;

        float filtered = 0.0f;

        int index = delayIndex_;

        for (int tap = 0; tap < numTaps_; ++tap)
        {
            filtered +=
                audioDelayLine_[index] *
                coefficients_[tap];

            --index;

            if (index < 0)
                index = numTaps_ - 1;
        }

        output.push_back(filtered);

        ++delayIndex_;

        if (delayIndex_ >= numTaps_)
            delayIndex_ = 0;
    }

    return output;
}


// // TODO: rename this file to LowPassFilter.cpp? 
// 
// #include "Filter.h"
// 
// #include <iostream>
// 
// LowPass::LowPass(float sampleRate, float cutoffFreq, float width)
// {
//     std::cout << "LowPass:LowPass()" << std::endl;
//     //float normalizedWidth = 
// }
// 
// 
// LowPass::~LowPass()
// {
//     // TODO: deconstructor
// }
// 
// float LowPass::processFloat()
// {
//     // float output = alpha * input + (1.0f - alpha) * prev_output;
//     // prev_output = output;
//     // return output;
//     return 0.0f;
// 
// }
// 
// float LowPass::processComplex()
// {
//     return 0.0f;
// 
// }
// 
// float LowPass::process()
// {
//     return processComplex();
// }
// 
// 
// 