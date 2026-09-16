#pragma once

#include "FIRDesign.h"
#include "FIRFilter.h"

// FIR high-pass filter (spectral inversion of a windowed-sinc low-pass).
class FIRHighPassFilter : public FIRFilter 
{
public:
    FIRHighPassFilter(int numTaps, double cutoffHz, double sampleRateHz) 
    {
        setCoefficients(FIRDesign::highPass(numTaps, cutoffHz, sampleRateHz));
    }
};
