#pragma once

#include "dsp/filters/FirDesign.h"
#include "dsp/filters/FirFilter.h"

// FIR band-pass filter: passes frequencies between lowHz and highHz.
class FirBandPassFilter : public FirFilter {
public:
    FirBandPassFilter(int numTaps, double lowHz, double highHz, double sampleRateHz) {
        setCoefficients(FirDesign::bandPass(numTaps, lowHz, highHz, sampleRateHz));
    }
};
