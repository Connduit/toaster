#pragma once

#include "dsp/filters/FirDesign.h"
#include "dsp/filters/FirFilter.h"

// FIR high-pass filter (spectral inversion of a windowed-sinc low-pass).
class FirHighPassFilter : public FirFilter {
public:
    FirHighPassFilter(int numTaps, double cutoffHz, double sampleRateHz) {
        setCoefficients(FirDesign::highPass(numTaps, cutoffHz, sampleRateHz));
    }
};
