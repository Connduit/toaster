#include "dsp/filters/IirFilter.h"

void IirFilter::setCoefficients(float b0, float b1, float b2, float a1, float a2) {
    feedforward0_ = b0;
    feedforward1_ = b1;
    feedforward2_ = b2;
    feedback1_ = a1;
    feedback2_ = a2;
}

float IirFilter::process(float sample) {
    float output = feedforward0_ * sample + feedforward1_ * previousInput1_ +
                    feedforward2_ * previousInput2_ - feedback1_ * previousOutput1_ -
                    feedback2_ * previousOutput2_;
    previousInput2_ = previousInput1_;
    previousInput1_ = sample;
    previousOutput2_ = previousOutput1_;
    previousOutput1_ = output;
    return output;
}

void IirFilter::reset() {
    previousInput1_ = previousInput2_ = previousOutput1_ = previousOutput2_ = 0.0f;
}
