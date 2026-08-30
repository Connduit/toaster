#ifndef TOASTER_DSP_H
#define TOASTER_DSP_H


#include <vector>
#include <complex>
#include <memory>

#pragma once
#include <vector>
#include <complex>

class DspProcessor {
public:
    explicit DspProcessor(float sample_rate = 48000.0f);
    ~DspProcessor(); 
    
    // Main processing function
    std::vector<float> process(const std::vector<std::complex<float>>& iq_samples);
    
    // Configuration
    void setLowpassCutoff(float cutoff);
    void setDeemphasisTimeConstant(float tau);
    void reset();
    
private:
    // Private filter classes (implementation hidden)
    class LowPass;
    class Deemphasis;
    
    std::unique_ptr<LowPass> lowpass_;
    std::unique_ptr<Deemphasis> deemphasis_;
    float sample_rate_;
    float cutoff_ = 200000.0f;
    float tau_ = 50e-6f;
};

// //#include "Filter.h"
// 
// #include <vector>
// #include <complex>
// 
// class DspProcessor
// {
// public:
//     std::vector<float> process(const std::vector<std::complex<float>>& iq_samples);
//     // Configuration
//     void setLowpassCutoff(float cutoff) { cutoff_ = cutoff; }
//     void setDeemphasisTimeConstant(float tau) { tau_ = tau; }
//     void reset();
// private:
//     // Private filter classes (implementation hidden)
//     class LowPass;
//     //class Deemphasis;
//     
//     //std::unique_ptr<LowPass> lowpass_;
//     LowPass* lowpass_;
// 
//     //std::unique_ptr<Deemphasis> deemphasis_;
//     float sample_rate_;
//     float cutoff_ = 200000.0f;
//     float tau_ = 50e-6f;
// };

#endif