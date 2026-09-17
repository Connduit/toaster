#ifndef TOASTER_DEMODULATOR_H
#define TOASTER_DEMODULATOR_H

#include "ToasterTypes.h"

// TODO: should be virtual class

// Idea is that Demodulator "strips" away carrier signal so ur just left with message signal
class Demodulator
{
public:
    Demodulator();
    AudioData process(const IQData& iqData);
    float process(float i, float q);
    float processSample(const std::complex<float>& current);

private:
    std::complex<float> previousIQ_{};
    bool hasPreviousIQ_ = false;
    float prevI_ = 0.0f;
    float prevQ_ = 0.0f;
    //std::complex<float> previousIQ_;
    //bool hasPreviousIQ_;
};

#endif