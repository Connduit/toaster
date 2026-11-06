#pragma once

#include "dsp/filters/IirDesign.h"
#include "dsp/filters/IirFilter.h"

// IIR high-pass filter (2nd-order Audio EQ Cookbook biquad).
class IirHighPassFilter : public IirFilter {
public:
    explicit IirHighPassFilter(double cutoffHz, double sampleRateHz,
                                double q = IirDesign::kButterworthQ) {
        auto c = IirDesign::highPass(cutoffHz, sampleRateHz, q);
        setCoefficients(c.b0, c.b1, c.b2, c.a1, c.a2);
    }
};
