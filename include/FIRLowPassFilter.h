#pragma once

#include "FIRDesign.h"
#include "FIRFilter.h"

// FIR low-pass filter (windowed-sinc, Hamming window).
class FIRLowPassFilter : public FIRFilter 
{
public:
    FIRLowPassFilter(int numTaps, double cutoffHz, double sampleRateHz)  // TODO: numTaps should be last param so i can give it default value
    {
        setCoefficients(FIRDesign::lowPass(numTaps, cutoffHz, sampleRateHz));
    }
};
