#pragma once

#include "IIRDesign.h"
#include "IIRFilter.h"

// IIR band-stop (notch) filter: rejects frequencies between lowHz and highHz.
class IIRBandStopFilter : public IIRFilter 
{
public:
    IIRBandStopFilter(double lowHz, double highHz, double sampleRateHz) 
    {
        auto c = IIRDesign::bandStop(lowHz, highHz, sampleRateHz);
        setCoefficients(c.b0, c.b1, c.b2, c.a1, c.a2);
    }
};
