#pragma once

#include "FIRDesign.h"
#include "FIRFilter.h"

// FIR band-stop (notch) filter: rejects frequencies between lowHz and highHz.
class FIRBandStopFilter : public FIRFilter 
{
public:
    FIRBandStopFilter(int numTaps, double lowHz, double highHz, double sampleRateHz) 
    {
        setCoefficients(FIRDesign::bandStop(numTaps, lowHz, highHz, sampleRateHz));
    }
};
