#pragma once

#include "dsp/filters/AdjustableLowPassFilter.h"
#include "dsp/filters/FirDesign.h"
#include "dsp/filters/FirFilter.h"

// FIR low-pass filter (windowed-sinc, Hamming window), with a cutoff
// that can be changed after construction (see setCutoffHz()).
//
// Implements AdjustableLowPassFilter via composition rather than
// inheriting FirFilter directly: both AdjustableLowPassFilter and
// FirFilter derive from Filter, and inheriting both here would need
// virtual inheritance to resolve the resulting diamond -- a pattern
// this project doesn't use anywhere else. Composition avoids that
// entirely: Impl (a private nested FirFilter) does the actual
// convolution, and this class just forwards process()/reset() to it and
// re-designs Impl's coefficients whenever the cutoff changes.
class FirLowPassFilter : public AdjustableLowPassFilter {
public:
    FirLowPassFilter(int numTaps, double cutoffHz, double sampleRateHz)
        : numTaps_(numTaps), sampleRateHz_(sampleRateHz) {
        setCutoffHz(cutoffHz);
    }

    float process(float sample) override { return impl_.process(sample); }
    void reset() override { impl_.reset(); }

    void setCutoffHz(double cutoffHz) override {
        impl_.setCoefficients(FirDesign::lowPass(numTaps_, cutoffHz, sampleRateHz_));
    }

private:
    // Exposes FirFilter's protected setCoefficients() to the outer
    // class -- safe here since Impl is a private implementation detail
    // nobody outside FirLowPassFilter can reach.
    class Impl : public FirFilter {
    public:
        using FirFilter::setCoefficients;
    };

    Impl impl_;
    int numTaps_;
    double sampleRateHz_;
};
