#pragma once

#include <vector>

// Windowed-sinc (Hamming window) FIR filter coefficient design.
//
// Low-pass is the base case. High-pass and band-stop are built by
// spectral inversion of a low-pass/band-pass design, which needs a
// well-defined center tap — so if you pass an even numTaps to highPass()
// or bandStop(), it's bumped up to the next odd number internally (the
// returned vector's length reflects this).
namespace FIRDesign 
{

std::vector<float> lowPass(int numTaps, double cutoffHz, double sampleRateHz);
std::vector<float> highPass(int numTaps, double cutoffHz, double sampleRateHz);

// Passes frequencies between lowHz and highHz.
std::vector<float> bandPass(int numTaps, double lowHz, double highHz, double sampleRateHz);

// Rejects (notches out) frequencies between lowHz and highHz.
std::vector<float> bandStop(int numTaps, double lowHz, double highHz, double sampleRateHz);

}  // namespace FirDesign
