#pragma once

#include "dsp/filters/AdjustableLowPassFilter.h"
#include "dsp/filters/IirDesign.h"
#include "dsp/filters/IirFilter.h"

// IIR low-pass filter (2nd-order Audio EQ Cookbook biquad), with a
// cutoff that can be changed after construction (see setCutoffHz()).
//
// Implemented via composition rather than inheriting IirFilter
// directly, for the same diamond-inheritance reason FirLowPassFilter
// gives in its own header comment.
class IirLowPassFilter : public AdjustableLowPassFilter {
public:
    explicit IirLowPassFilter(double cutoffHz, double sampleRateHz, double q = IirDesign::kButterworthQ)
        : sampleRateHz_(sampleRateHz), q_(q) {
        setCutoffHz(cutoffHz);
    }

    float process(float sample) override { return impl_.process(sample); }
    void reset() override { impl_.reset(); }

    void setCutoffHz(double cutoffHz) override {
        auto c = IirDesign::lowPass(cutoffHz, sampleRateHz_, q_);
        impl_.setCoefficients(c.b0, c.b1, c.b2, c.a1, c.a2);
    }

private:
    class Impl : public IirFilter {
    public:
        using IirFilter::setCoefficients;
    };

    Impl impl_;
    double sampleRateHz_;
    double q_;
};
