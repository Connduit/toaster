#ifndef TOASTER_PCM_SINK_H
#define TOASTER_PCM_SINK_H

#include "AudioSink.h"

class PcmSink : public AudioSink 
{
public:
    explicit PcmSink(
                FILE* stream = stdout, 
                float gain = 1.0f, size_t reserveSamples = 4096)
                : 
                stream_(stream), 
                gain_(gain) 
    {
        buffer_.reserve(reserveSamples);
    }

    // Convert and buffer one sample. `sample` should be roughly in [-1, 1]
    // before gain is applied.
    void pushSample(float sample) override 
    {
        float scaled = sample * gain_;
        // TODO: use std::clamp instead?
        if (scaled > 32767.0f) scaled = 32767.0f;
        if (scaled < -32768.0f) scaled = -32768.0f;
        buffer_.push_back(static_cast<int16_t>(scaled));
    }

    // Flush any buffered samples out to the stream.
    void flush() override 
    {
        if (buffer_.empty()) 
        {
            return;
        }
        fwrite(buffer_.data(), sizeof(int16_t), buffer_.size(), stream_);
        fflush(stream_);
        buffer_.clear();
    }

    void setGain(float gain) { gain_ = gain; }

private:
    FILE* stream_;
    float gain_;
    std::vector<int16_t> buffer_;
};

#endif
