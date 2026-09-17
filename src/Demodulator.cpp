#include "Demodulator.h"
#include <cmath>

Demodulator::Demodulator()
    : previousIQ_(0.0f, 0.0f),
      hasPreviousIQ_(false)
{
}

// FmDemodulator
float Demodulator::process(float i, float q)
{
    float re = i * prevI_ + q * prevQ_;
    float im = q * prevI_ - i * prevQ_;

    prevI_ = i;
    prevQ_ = q;

    return std::atan2(im, re);

}
