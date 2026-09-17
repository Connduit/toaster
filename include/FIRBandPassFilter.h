#pragma once

#include "FIRDesign.h"
#include "FIRFilter.h"

// FIR band-pass filter: passes frequencies between lowHz and highHz.
class FIRBandPassFilter : public FIRFilter 
{
public:
    FIRBandPassFilter(int numTaps, double lowHz, double highHz, double sampleRateHz) 
    {
        setCoefficients(FIRDesign::bandPass(numTaps, lowHz, highHz, sampleRateHz));
    }
};
