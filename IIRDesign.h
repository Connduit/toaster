#pragma once

// Biquad (2nd-order IIR) coefficient design using the standard "Audio EQ
// Cookbook" (RBJ) formulas.
namespace IIRDesign {

// a0 has already been divided out, so these plug straight into
// IirFilter::setCoefficients(b0, b1, b2, a1, a2).
struct Coefficients {
    float b0, b1, b2, a1, a2;
};

// Q = 1/sqrt(2) (~0.7071) gives a maximally-flat, Butterworth-like
// response, which is a reasonable default if you don't care about the
// resonance/rolloff tradeoff.
constexpr double kButterworthQ = 0.70710678;

Coefficients lowPass(double cutoffHz, double sampleRateHz, double q = kButterworthQ);
Coefficients highPass(double cutoffHz, double sampleRateHz, double q = kButterworthQ);

// Center frequency is the geometric mean of lowHz/highHz; Q is derived
// from center / (highHz - lowHz), so the passband/stopband roughly
// spans [lowHz, highHz] the same way the FIR band-pass/band-stop do.
Coefficients bandPass(double lowHz, double highHz, double sampleRateHz);
Coefficients bandStop(double lowHz, double highHz, double sampleRateHz);

}  // namespace IirDesign
