#ifndef TOASTER_IIR_FILTER_H
#define TOASTER_IIR_FILTER_H

#include "Filter.h"

class IIRFilter : public Filter 
{
public:
    // lowpass
    IIRFilter(float cutoffFrequency, float sampleRate);

    float process(float input) override;

    AudioData process(const IQData& iqData);

private:
    float alpha_;
    float state_;
};

#endif