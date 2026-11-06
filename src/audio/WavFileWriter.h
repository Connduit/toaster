#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "audio/AudioSink.h"

// Writes 16-bit PCM samples to a standard RIFF/WAVE (.wav) file.
//
// The header is written with placeholder sizes up front, then patched
// with the real sizes once the total sample count is known (on flush()
// and again on destruction), since WAV headers need the total data size
// up front but we're streaming samples in as they arrive.
class WavFileWriter : public AudioSink {
public:
    // path         - output .wav file path
    // sampleRateHz - audio sample rate to record in the header
    // numChannels  - 1 for mono, 2 for stereo (this project only produces mono)
    // gain         - applied before clipping to int16, same role as PcmWriter's gain
    WavFileWriter(const std::string& path, uint32_t sampleRateHz, int numChannels = 1,
                  float gain = 1.0f);
    ~WavFileWriter() override;

    // Non-copyable: owns a FILE*.
    WavFileWriter(const WavFileWriter&) = delete;
    WavFileWriter& operator=(const WavFileWriter&) = delete;

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
