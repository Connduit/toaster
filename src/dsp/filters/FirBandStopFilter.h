#pragma once

#include "dsp/filters/FirDesign.h"
#include "dsp/filters/FirFilter.h"

// FIR band-stop (notch) filter: rejects frequencies between lowHz and highHz.
class FirBandStopFilter : public FirFilter {
public:
    FirBandStopFilter(int numTaps, double lowHz, double highHz, double sampleRateHz) {
        setCoefficients(FirDesign::bandStop(numTaps, lowHz, highHz, sampleRateHz));
    }
};
