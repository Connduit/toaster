#pragma once

#include "dsp/filters/Filter.h"

// A Filter that's specifically a low-pass, with a cutoff frequency that
// can be changed after construction.
//
// This exists for exactly one consumer: AudioStage's anti-aliasing
// filter, which is always conceptually "a low-pass with some cutoff"
// regardless of whether -t picked FIR or IIR underneath. Letting the
// cutoff change live -- from a console "bandwidth <hz>" command, say --
// is what gives AudioStage a gqrx-style adjustable audio filter width
// without it needing to know or care which concrete filter type it's
// holding.
//
// Deliberately narrower than a general "rebuild any filter shape at
// runtime" facility: high-pass/band-pass/band-stop filters don't have a
// single "cutoff" (they have a low and a high edge, or a stop, or all
// three), so forcing this method onto every Filter subclass would be a
// poor fit for those. Only the low-pass concrete classes implement this.
class AdjustableLowPassFilter : public Filter {
public:
    // Recomputes the filter's coefficients for a new cutoff frequency,
    // keeping tap count/sample rate/Q exactly as they were at
    // construction. Also clears the filter's history, same as reset()
    // would -- a cutoff change is itself a discontinuity in the
    // filter's transfer function, so history computed under the old
    // cutoff doesn't mean anything under the new one.
    virtual void setCutoffHz(double cutoffHz) = 0;
};
