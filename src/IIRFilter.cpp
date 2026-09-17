#include "IIRFilter.h" 

void IIRFilter::setCoefficients(float b0, float b1, float b2, float a1, float a2) 
{
    b0_ = b0;
    b1_ = b1;
    b2_ = b2;
    a1_ = a1;
    a2_ = a2;
}

float IIRFilter::process(float sample) 
{
    float output = b0_ * sample + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;
    x2_ = x1_;
    x1_ = sample;
    y2_ = y1_;
    y1_ = output;
    return output;
}

void IIRFilter::reset() { x1_ = x2_ = y1_ = y2_ = 0.0f; }


