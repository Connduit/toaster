#pragma once

#include "FIRDesign.h"
#include "FIRFilter.h"

// FIR low-pass filter (windowed-sinc, Hamming window).
class FIRLowPassFilter : public FIRFilter 
{
public:
    FIRLowPassFilter(int numTaps, double cutoffHz, double sampleRateHz) 
    {
        setCoefficients(FIRDesign::lowPass(numTaps, cutoffHz, sampleRateHz));
    }
};
