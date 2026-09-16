#ifndef TOASTER_DECIMATOR_H
#define TOASTER_DECIMATOR_H

#include "ToasterTypes.h"

// Throws away samples
class Decimator {
public:
    explicit Decimator(int factor) : factor_(factor) {}

    // Push one sample. If this call lands on a kept sample, writes it to
    // out and returns true. Otherwise returns false and out is untouched.
    bool push(float sample, float& out) {
        if (++counter_ < factor_) return false;
        counter_ = 0;
        out = sample;
        return true;
    }

    void reset() { counter_ = 0; }

private:
    int factor_;
    int counter_ = 0;
};
//class Decimator
//{
//public:
//
//    explicit Decimator(int factor);
//
//    AudioData process(const AudioData& input);
//    bool processSample(float input, float &output);
//
//private:
//    int factor_;
//    int counter_;
//
//    // Filter state
//    //float previousSample_;
//};

#endif