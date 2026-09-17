#ifndef TOASTER_WAV_SINK_H
#define TOASTER_WAV_SINK_H

#include "AudioSink.h"

class WavSink : public AudioSink 
{
public:
    
    //WavSink();
    // path         - output .wav file path
    // sampleRateHz - audio sample rate to record in the header
    // numChannels  - 1 for mono, 2 for stereo (this project only produces mono)
    // gain         - applied before clipping to int16, same role as PcmWriter's gain
    WavSink(const std::string& path = "output.wav", uint32_t sampleRateHz = 48000, int numChannels = 1, float gain = 1.0f);
    ~WavSink();

    WavSink(const WavSink &) = delete;
    WavSink& operator=(const WavSink&) = delete;

    void pushSample(float sample) override;
    void flush() override;

    // False if the file could not be opened; pushSample()/flush() are
    // then safe no-ops rather than crashes.
    bool isOpen() const { return file_ != nullptr; }

private:
    void writePlaceholderHeader();
    void patchHeaderSizes();

    FILE* file_ = nullptr;
    uint32_t sampleRateHz_;
    int numChannels_;
    float gain_;
    std::vector<int16_t> buffer_;
    uint64_t totalSamplesWritten_ = 0;
};

#endif
