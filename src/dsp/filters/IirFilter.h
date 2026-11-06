#pragma once

#include "dsp/filters/Filter.h"

// Base class for IIR (infinite impulse response) filters, implemented as
// a single second-order section ("biquad"). Implements the direct-form-I
// difference equation every biquad needs:
//
//   y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
//
// regardless of what shape (low-pass, high-pass, band-pass, band-stop)
// its coefficients describe. Concrete subclasses just design coefficients
// (see IirDesign.h, which uses the standard "Audio EQ Cookbook" formulas)
// and hand them to setCoefficients() from their constructor.
//
// Not meant to be instantiated directly: use one of the IirLowPassFilter
// / IirHighPassFilter / IirBandPassFilter / IirBandStopFilter subclasses.
class IirFilter : public Filter {
public:
    float process(float sample) override;
    void reset() override;

protected:
    IirFilter() = default;

    // a0 is assumed to already be normalized to 1 (i.e. b0..a2 have all
    // been divided by the original a0) before calling this — that's what
    // IirDesign's coefficient functions return.
    void setCoefficients(float b0, float b1, float b2, float a1, float a2);

private:
    // "feedforward" (b) coefficients weight the current/past inputs;
    // "feedback" (a) coefficients weight the past outputs. a0 is assumed
    // already divided out (see setCoefficients above).
    float feedforward0_ = 1.0f, feedforward1_ = 0.0f, feedforward2_ = 0.0f;
    float feedback1_ = 0.0f, feedback2_ = 0.0f;
    float previousInput1_ = 0.0f, previousInput2_ = 0.0f;    // x[n-1], x[n-2]
    float previousOutput1_ = 0.0f, previousOutput2_ = 0.0f;  // y[n-1], y[n-2]
};
