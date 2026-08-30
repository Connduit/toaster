#include "DspProcessor.h"

#include <cmath>
#include <stdexcept>
#include <algorithm>

// ============================================================
// INNER CLASS: LowPass (FIR Windowed Sinc)
// ============================================================

class DspProcessor::LowPass {
private:
    std::vector<float> coefficients;
    std::vector<std::complex<float>> delay_line;
    
public:
    LowPass(float sample_rate, float cutoff_freq, float transition_width = 50000.0f) {
        // --- Step 1: Calculate number of taps ---
        float normalized_transition = transition_width / sample_rate;
        int num_taps = static_cast<int>(std::ceil(4.0f / normalized_transition));
        
        if (num_taps % 2 == 0) num_taps++;
        if (num_taps > 1000) num_taps = 1000;
        
        coefficients.resize(num_taps);
        delay_line.resize(num_taps, std::complex<float>(0.0f, 0.0f));
        
        // --- Step 2: Design filter (Windowed Sinc) ---
        float normalized_cutoff = cutoff_freq / sample_rate;
        
        for (int i = 0; i < num_taps; ++i) {
            float t = static_cast<float>(i) - static_cast<float>(num_taps / 2);
            
            float sinc;
            if (std::abs(t) < 0.0001f) {
                sinc = 2.0f * normalized_cutoff;
            } else {
                sinc = std::sin(2.0f * M_PI * normalized_cutoff * t) / (M_PI * t);
            }
            
            // Hamming window
            float hamming = 0.54f - 0.46f * std::cos(2.0f * M_PI * i / (num_taps - 1));
            coefficients[i] = sinc * hamming;
        }
        
        // Normalize
        float sum = 0.0f;
        for (float coeff : coefficients) sum += coeff;
        for (float& coeff : coefficients) coeff /= sum;
    }
    
    std::complex<float> process(const std::complex<float>& input) {
        // Shift delay line
        for (size_t i = delay_line.size() - 1; i > 0; --i) {
            delay_line[i] = delay_line[i - 1];
        }
        delay_line[0] = input;
        
        // Convolution
        std::complex<float> output(0.0f, 0.0f);
        for (size_t i = 0; i < coefficients.size(); ++i) {
            output += delay_line[i] * coefficients[i];
        }
        return output;
    }
    
    void reset() {
        std::fill(delay_line.begin(), delay_line.end(), std::complex<float>(0.0f, 0.0f));
    }
};

// ============================================================
// INNER CLASS: Deemphasis (IIR Single-Pole)
// ============================================================

class DspProcessor::Deemphasis {
private:
    float prev_output = 0.0f;
    float alpha = 0.0f;
    
public:
    Deemphasis(float sample_rate, float tau = 50e-6f) {
        // y[n] = alpha * x[n] + (1-alpha) * y[n-1]
        alpha = 1.0f / (1.0f + 1.0f / (2.0f * M_PI * tau * sample_rate));
    }
    
    float process(float input) {
        float output = alpha * input + (1.0f - alpha) * prev_output;
        prev_output = output;
        return output;
    }
    
    void reset() {
        prev_output = 0.0f;
    }
};

// ============================================================
// DSPROCESSOR IMPLEMENTATION
// ============================================================

DspProcessor::DspProcessor(float sample_rate)
    : lowpass_(std::make_unique<LowPass>(sample_rate, cutoff_))
    , deemphasis_(std::make_unique<Deemphasis>(sample_rate))
    , sample_rate_(sample_rate) {
    // Body can be empty
}

// ✅ DESTRUCTOR - MUST be defined here where inner classes are complete!
DspProcessor::~DspProcessor() = default;

// ============================================================
// MAIN PROCESSING FUNCTION
// ============================================================

std::vector<float> DspProcessor::process(const std::vector<std::complex<float>>& iq_samples) {
    if (iq_samples.empty()) {
        return {};
    }
    
    // --- Stage 1: Lowpass filter (FIR) ---
    // Processes complex IQ samples
    std::vector<std::complex<float>> filtered;
    filtered.reserve(iq_samples.size());
    
    for (const auto& sample : iq_samples) {
        filtered.push_back(lowpass_->process(sample));
    }
    
    // --- Stage 2: FM Demodulation ---
    // Converts complex IQ to float audio by extracting phase derivative
    std::vector<float> audio;
    audio.reserve(filtered.size() - 1);
    
    for (size_t i = 1; i < filtered.size(); ++i) {
        // Get phase of current and previous sample
        float phase_now = std::arg(filtered[i]);
        float phase_prev = std::arg(filtered[i - 1]);
        
        // Phase difference = frequency = audio!
        float diff = phase_now - phase_prev;
        
        // Unwrap phase to handle jumps beyond [-PI, PI]
        while (diff > M_PI) diff -= 2.0f * M_PI;
        while (diff < -M_PI) diff += 2.0f * M_PI;
        
        audio.push_back(diff);
    }
    
    // --- Stage 3: De-emphasis filter (IIR) ---
    // Gently reduces high frequencies to make audio sound natural
    for (auto& sample : audio) {
        sample = deemphasis_->process(sample);
    }
    
    return audio;
}

// ============================================================
// CONFIGURATION METHODS
// ============================================================

void DspProcessor::setLowpassCutoff(float cutoff) {
    cutoff_ = cutoff;
    // Recreate the filter with new cutoff
    lowpass_ = std::make_unique<LowPass>(sample_rate_, cutoff_);
}

void DspProcessor::setDeemphasisTimeConstant(float tau) {
    tau_ = tau;
    // Recreate the filter with new time constant
    deemphasis_ = std::make_unique<Deemphasis>(sample_rate_, tau_);
}

void DspProcessor::reset() {
    if (lowpass_) lowpass_->reset();
    if (deemphasis_) deemphasis_->reset();
}