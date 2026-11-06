#pragma once

#include "dsp/filters/IirDesign.h"
#include "dsp/filters/IirFilter.h"

// IIR band-pass filter: passes frequencies between lowHz and highHz.
class IirBandPassFilter : public IirFilter {
public:
    IirBandPassFilter(double lowHz, double highHz, double sampleRateHz) {
        auto c = IirDesign::bandPass(lowHz, highHz, sampleRateHz);
        setCoefficients(c.b0, c.b1, c.b2, c.a1, c.a2);
    }
};
