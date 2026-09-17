#pragma once

#include "IIRDesign.h"
#include "IIRFilter.h"

// IIR band-pass filter: passes frequencies between lowHz and highHz.
class IIRBandPassFilter : public IIRFilter 
{
public:
    IIRBandPassFilter(double lowHz, double highHz, double sampleRateHz) 
    {
        auto c = IIRDesign::bandPass(lowHz, highHz, sampleRateHz);
        setCoefficients(c.b0, c.b1, c.b2, c.a1, c.a2);
    }
};
