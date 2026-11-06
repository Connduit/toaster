#pragma once

// Abstract base class for any single-sample-in, single-sample-out
// filter. Every concrete filter in this project — FIR or IIR, low-pass,
// high-pass, band-pass, or band-stop — implements this same tiny
// interface, so callers (like AudioPipeline) can hold any of them
// through a Filter*, or a std::unique_ptr<Filter>, without caring which
// concrete type it is.
class Filter {
public:
    virtual ~Filter() = default;

    // Filter one input sample, return one output sample.
    virtual float process(float sample) = 0;

    // Clear internal state (history/delay lines) back to zero.
    virtual void reset() = 0;
};
