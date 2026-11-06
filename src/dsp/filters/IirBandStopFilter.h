#pragma once

#include "dsp/filters/IirDesign.h"
#include "dsp/filters/IirFilter.h"

// IIR band-stop (notch) filter: rejects frequencies between lowHz and highHz.
class IirBandStopFilter : public IirFilter {
public:
    IirBandStopFilter(double lowHz, double highHz, double sampleRateHz) {
        auto c = IirDesign::bandStop(lowHz, highHz, sampleRateHz);
        setCoefficients(c.b0, c.b1, c.b2, c.a1, c.a2);
    }
};
