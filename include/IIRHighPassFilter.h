#pragma once

#include "IIRDesign.h"
#include "IIRFilter.h"

// IIR high-pass filter (2nd-order Audio EQ Cookbook biquad).
class IIRHighPassFilter : public IIRFilter 
{
public:
    explicit IIRHighPassFilter(double cutoffHz, double sampleRateHz, double q = IIRDesign::kButterworthQ) 
    {
        auto c = IIRDesign::highPass(cutoffHz, sampleRateHz, q);
        setCoefficients(c.b0, c.b1, c.b2, c.a1, c.a2);
    }
};
