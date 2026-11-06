#pragma once

// A plain downsampler: keeps 1 out of every `factor` samples it's given and
// discards the rest. It has no idea what a filter is — anti-aliasing is
// the caller's job (e.g. run samples through a LowPassFilter first).
class Decimator {
public:
    explicit Decimator(int factor) : factor_(factor) {}

    // Push one sample. If this call lands on a kept sample, writes it to
    // out and returns true. Otherwise returns false and out is untouched.
    bool push(float sample, float& out) {
        if (++counter_ < factor_) return false;
        counter_ = 0;
        out = sample;
        return true;
    }

    void reset() { counter_ = 0; }

private:
    int factor_;
    int counter_ = 0;
};
