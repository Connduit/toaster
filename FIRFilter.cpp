#include "FIRFilter.h"

#include <algorithm>

void FIRFilter::setCoefficients(std::vector<float> coefficients) 
{
    coeffs_ = std::move(coefficients);
    history_.assign(coeffs_.size(), 0.0f);
    historyPos_ = 0;
}

float FIRFilter::process(float sample) 
{
    if (coeffs_.empty())
    {
        return sample;  // never configured; pass through
    }

    history_[historyPos_] = sample;
    historyPos_ = (historyPos_ + 1) % static_cast<int>(history_.size());

    // historyPos_ currently points at the oldest sample in the ring
    // buffer, which lines up with the start of the coefficient array.
    float acc = 0.0f;
    int idx = historyPos_;
    for (size_t n = 0; n < coeffs_.size(); ++n) 
    {
        acc += history_[idx] * coeffs_[n];
        idx = (idx + 1) % static_cast<int>(history_.size());
    }
    return acc;
}

void FIRFilter::reset() 
{
    std::fill(history_.begin(), history_.end(), 0.0f);
    historyPos_ = 0;
}
