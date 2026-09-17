#pragma once

#include "IIRDesign.h"
#include "IIRFilter.h"

// IIR low-pass filter (2nd-order Audio EQ Cookbook biquad).
class IIRLowPassFilter : public IIRFilter 
{
public:
    explicit IIRLowPassFilter(double cutoffHz, double sampleRateHz, double q = IIRDesign::kButterworthQ) 
    {
        auto c = IIRDesign::lowPass(cutoffHz, sampleRateHz, q);
        setCoefficients(c.b0, c.b1, c.b2, c.a1, c.a2);
    }
};
