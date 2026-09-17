#ifndef TOASTER_IIR_FILTER_H
#define TOASTER_IIR_FILTER_H

#include "Filter.h"

class IIRFilter : public Filter 
{
public:
    float process(float sample) override;
    void reset() override;

protected:
    IIRFilter() = default;

    // a0 is assumed to already be normalized to 1 (i.e. b0..a2 have all
    // been divided by the original a0) before calling this — that's what
    // IirDesign's coefficient functions return.
    void setCoefficients(float b0, float b1, float b2, float a1, float a2);

private:
    float b0_ = 1.0f, b1_ = 0.0f, b2_ = 0.0f;
    float a1_ = 0.0f, a2_ = 0.0f;
    float x1_ = 0.0f, x2_ = 0.0f;  // previous two inputs
    float y1_ = 0.0f, y2_ = 0.0f;  // previous two outputs
};

#endif