#include "IIRFilter.h" 


IIRFilter::IIRFilter(float cutoffFrequency, float sampleRate)
    : 
    alpha_(0.0f),
    state_(0.0f)
{
    const float rc = 1.0f / (2.0f * static_cast<float>(M_PI) * cutoffFrequency);
    const float dt = 1.0f / sampleRate;
    alpha_ = dt / (rc + dt);
}

AudioData IIRFilter::process(const IQData& iqData)
{
    AudioData audio;
    return audio;
}

float IIRFilter::process(float input)
{
    state_ += alpha_ * (input - state_);

    return state_;
}