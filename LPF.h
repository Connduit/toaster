#pragma once

class LPF
{
public:
    LPF(float cutoffFrequency, float sampleRate);

    float process(float input);

private:
    float alpha_;
    float state_;
};