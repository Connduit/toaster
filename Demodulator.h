#ifndef TOASTER_DEMODULATOR_H
#define TOASTER_DEMODULATOR_H

#include "ToasterTypes.h"

// class FMDemodulator
class Demodulator
{
public:
    Demodulator();
    AudioData process(const IQData& iqData);
    float processSample(const std::complex<float>& current);

private:
    std::complex<float> previousIQ_{};
    bool hasPreviousIQ_ = false;
    //std::complex<float> previousIQ_;
    //bool hasPreviousIQ_;
};

#endif