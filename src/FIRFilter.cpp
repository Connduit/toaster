#include "FIRFilter.h"

#include <algorithm>

void FIRFilter::setCoefficients(std::vector<float> coefficients) 
{
    coeffs_ = std::move(coefficients);
    numTaps_ = static_cast<int>(coeffs_.size());
    // Double-length so every logical position has a mirror at +numTaps_,
    // letting process() read a contiguous window with no wraparound.
    history_.assign(static_cast<size_t>(numTaps_) * 2, 0.0f);
    historyPos_ = 0;
}

float FIRFilter::process(float sample) 
{
    if (coeffs_.empty())
    {

        // TODO: call setCoffs(default coffs?)
        return sample; // never configured; pass through
    }

    // Write to both halves of the doubled buffer so that
    // history_[historyPos_ .. historyPos_ + numTaps_ - 1] is always a
    // valid, contiguous, correctly-ordered window after advancing below
    // — equivalent to the old history_[idx % numTaps_] scheme, but
    // computed once per sample instead of once per tap.
    history_[historyPos_] = sample;
    history_[static_cast<size_t>(historyPos_) + numTaps_] = sample;

    historyPos_ = (historyPos_ + 1) % numTaps_;

    // Contiguous read, oldest sample first, newest sample last — no
    // modulo, no branching, and this loop auto-vectorizes.
    const float* window = &history_[static_cast<size_t>(historyPos_)];
    float acc = 0.0f;
    for (int n = 0; n < numTaps_; ++n) 
    {
        acc += window[n] * coeffs_[n];
    }
    return acc;
}

void FIRFilter::reset() 
{
    std::fill(history_.begin(), history_.end(), 0.0f);
    historyPos_ = 0;
}