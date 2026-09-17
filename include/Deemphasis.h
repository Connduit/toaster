// TODO? 
#pragma once

#include "Filter.h"

// Single-pole IIR de-emphasis filter, as used to undo the pre-emphasis
// applied by FM broadcast transmitters. Implements Filter for
// consistency with the rest of the filter hierarchy, even though it's
// a simpler one-pole shelf rather than one of the LP/HP/BP/BS shapes.
//
// Time constant is typically 75us (US, South Korea) or 50us (most of the
// rest of the world, including Europe).
class DeemphasisFilter : public Filter {
public:
    DeemphasisFilter(double sampleRateHz, double tauMicroseconds) {
        double dt = 1.0 / sampleRateHz;
        double rc = tauMicroseconds * 1e-6;
        alpha_ = static_cast<float>(dt / (rc + dt));
    }

    float process(float sample) override {
        prev_ += alpha_ * (sample - prev_);
        return prev_;
    }

    void reset() override { prev_ = 0.0f; }

private:
    float alpha_;
    float prev_ = 0.0f;
};
