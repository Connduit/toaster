#include "FIRDesign.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace 
{

int makeOdd(int numTaps) { return (numTaps % 2 == 0) ? numTaps + 1 : numTaps; }

// Core building block: a windowed-sinc low-pass, normalized to unity
// gain at DC. Every other shape here is built from one or two of these.
std::vector<float> windowedSincLowpass(int numTaps, double cutoffHz, double sampleRateHz) 
{
    std::vector<float> h(numTaps);
    double fc = cutoffHz / sampleRateHz;  // normalized cutoff (0..0.5)
    int m = numTaps - 1;

    double sum = 0.0;
    for (int n = 0; n < numTaps; ++n) {
        double x = n - m / 2.0;
        double sinc = (x == 0.0) ? 2.0 * fc : std::sin(2.0 * M_PI * fc * x) / (M_PI * x);
        double window = 0.54 - 0.46 * std::cos(2.0 * M_PI * n / m);  // Hamming window
        double val = sinc * window;
        h[n] = static_cast<float>(val);
        sum += val;
    }
    for (int n = 0; n < numTaps; ++n) h[n] = static_cast<float>(h[n] / sum);
    return h;
}

}  // namespace

namespace FIRDesign 
{

std::vector<float> lowPass(int numTaps, double cutoffHz, double sampleRateHz) 
{
    return windowedSincLowpass(numTaps, cutoffHz, sampleRateHz);
}

std::vector<float> highPass(int numTaps, double cutoffHz, double sampleRateHz) 
{
    numTaps = makeOdd(numTaps);
    std::vector<float> lp = windowedSincLowpass(numTaps, cutoffHz, sampleRateHz);

    // Spectral inversion: subtract the low-pass response from an all-pass
    // (a single 1.0 at the center tap). What the low-pass rejects, this
    // passes, and vice versa.
    std::vector<float> hp(numTaps);
    for (int n = 0; n < numTaps; ++n) hp[n] = -lp[n];
    hp[numTaps / 2] += 1.0f;
    return hp;
}

std::vector<float> bandPass(int numTaps, double lowHz, double highHz, double sampleRateHz) 
{
    // A band-pass is what's left when you take a low-pass at the high
    // edge and remove everything a low-pass at the low edge would also
    // pass — i.e. their difference.
    std::vector<float> lpHigh = windowedSincLowpass(numTaps, highHz, sampleRateHz);
    std::vector<float> lpLow = windowedSincLowpass(numTaps, lowHz, sampleRateHz);

    std::vector<float> bp(numTaps);
    for (int n = 0; n < numTaps; ++n) bp[n] = lpHigh[n] - lpLow[n];
    return bp;
}

std::vector<float> bandStop(int numTaps, double lowHz, double highHz, double sampleRateHz) 
{
    numTaps = makeOdd(numTaps);
    std::vector<float> bp = bandPass(numTaps, lowHz, highHz, sampleRateHz);

    // Spectral inversion of the band-pass: reject what it passes.
    std::vector<float> bs(numTaps);
    for (int n = 0; n < numTaps; ++n) bs[n] = -bp[n];
    bs[numTaps / 2] += 1.0f;
    return bs;
}

}  // namespace FirDesign
