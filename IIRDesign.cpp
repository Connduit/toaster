#include "IIRDesign.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

// Shared setup every RBJ biquad formula starts from.
struct Trig {
    double cosw0;
    double alpha;
};

Trig computeTrig(double freqHz, double sampleRateHz, double q) {
    double w0 = 2.0 * M_PI * freqHz / sampleRateHz;
    return {std::cos(w0), std::sin(w0) / (2.0 * q)};
}

IIRDesign::Coefficients normalize(double b0, double b1, double b2, double a0, double a1,
                                   double a2) {
    return {static_cast<float>(b0 / a0), static_cast<float>(b1 / a0), static_cast<float>(b2 / a0),
            static_cast<float>(a1 / a0), static_cast<float>(a2 / a0)};
}

double centerFrequency(double lowHz, double highHz) { return std::sqrt(lowHz * highHz); }
double qFromBandwidth(double lowHz, double highHz) {
    return centerFrequency(lowHz, highHz) / (highHz - lowHz);
}

}  // namespace

namespace IIRDesign {

Coefficients lowPass(double cutoffHz, double sampleRateHz, double q) {
    Trig t = computeTrig(cutoffHz, sampleRateHz, q);
    double b0 = (1.0 - t.cosw0) / 2.0;
    double b1 = 1.0 - t.cosw0;
    double b2 = (1.0 - t.cosw0) / 2.0;
    double a0 = 1.0 + t.alpha;
    double a1 = -2.0 * t.cosw0;
    double a2 = 1.0 - t.alpha;
    return normalize(b0, b1, b2, a0, a1, a2);
}

Coefficients highPass(double cutoffHz, double sampleRateHz, double q) {
    Trig t = computeTrig(cutoffHz, sampleRateHz, q);
    double b0 = (1.0 + t.cosw0) / 2.0;
    double b1 = -(1.0 + t.cosw0);
    double b2 = (1.0 + t.cosw0) / 2.0;
    double a0 = 1.0 + t.alpha;
    double a1 = -2.0 * t.cosw0;
    double a2 = 1.0 - t.alpha;
    return normalize(b0, b1, b2, a0, a1, a2);
}

Coefficients bandPass(double lowHz, double highHz, double sampleRateHz) {
    double center = centerFrequency(lowHz, highHz);
    double q = qFromBandwidth(lowHz, highHz);
    Trig t = computeTrig(center, sampleRateHz, q);

    // Constant 0dB peak-gain variant.
    double b0 = t.alpha;
    double b1 = 0.0;
    double b2 = -t.alpha;
    double a0 = 1.0 + t.alpha;
    double a1 = -2.0 * t.cosw0;
    double a2 = 1.0 - t.alpha;
    return normalize(b0, b1, b2, a0, a1, a2);
}

Coefficients bandStop(double lowHz, double highHz, double sampleRateHz) {
    double center = centerFrequency(lowHz, highHz);
    double q = qFromBandwidth(lowHz, highHz);
    Trig t = computeTrig(center, sampleRateHz, q);

    double b0 = 1.0;
    double b1 = -2.0 * t.cosw0;
    double b2 = 1.0;
    double a0 = 1.0 + t.alpha;
    double a1 = -2.0 * t.cosw0;
    double a2 = 1.0 - t.alpha;
    return normalize(b0, b1, b2, a0, a1, a2);
}

}  // namespace IIRDesign
