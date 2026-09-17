#include "WavSink.h"
#include <iostream>

namespace {

// WAV headers store everything little-endian regardless of host byte
// order, so write bytes explicitly instead of assuming host endianness.
void writeU32LE(FILE* f, uint32_t value) {
    unsigned char bytes[4] = {
        static_cast<unsigned char>(value & 0xFF),
        static_cast<unsigned char>((value >> 8) & 0xFF),
        static_cast<unsigned char>((value >> 16) & 0xFF),
        static_cast<unsigned char>((value >> 24) & 0xFF),
    };
    fwrite(bytes, 1, 4, f);
}

void writeU16LE(FILE* f, uint16_t value) {
    unsigned char bytes[2] = {
        static_cast<unsigned char>(value & 0xFF),
        static_cast<unsigned char>((value >> 8) & 0xFF),
    };
    fwrite(bytes, 1, 2, f);
}

constexpr size_t kFlushThresholdSamples = 4096;
constexpr int kBitsPerSample = 16;

}  // namespace

WavSink::WavSink(const std::string& path, uint32_t sampleRateHz, int numChannels,
                              float gain)
    : sampleRateHz_(sampleRateHz), numChannels_(numChannels), gain_(gain) {
    buffer_.reserve(kFlushThresholdSamples);

    file_ = fopen(path.c_str(), "wb");
    if (!file_) {
        fprintf(stderr, "Failed to open '%s' for writing.\n", path.c_str());
        return;
    }
    writePlaceholderHeader();
}

WavSink::~WavSink()
{
    if (file_)
    {
        flush();

        patchHeaderSizes();

        fclose(file_);

        file_ = nullptr;
    }
}

void WavSink::writePlaceholderHeader() {
    // Standard 44-byte canonical PCM WAV header. ChunkSize and
    // Subchunk2Size (data size) are unknown until we're done, so they're
    // written as 0 here and patched in patchHeaderSizes().
    fwrite("RIFF", 1, 4, file_);
    writeU32LE(file_, 0);  // ChunkSize placeholder
    fwrite("WAVE", 1, 4, file_);

    fwrite("fmt ", 1, 4, file_);
    writeU32LE(file_, 16);  // Subchunk1Size (16 for PCM)
    writeU16LE(file_, 1);   // AudioFormat = 1 (PCM, no compression)
    writeU16LE(file_, static_cast<uint16_t>(numChannels_));
    writeU32LE(file_, sampleRateHz_);

    uint32_t byteRate = sampleRateHz_ * numChannels_ * (kBitsPerSample / 8);
    writeU32LE(file_, byteRate);

    uint16_t blockAlign = static_cast<uint16_t>(numChannels_ * (kBitsPerSample / 8));
    writeU16LE(file_, blockAlign);
    writeU16LE(file_, static_cast<uint16_t>(kBitsPerSample));

    fwrite("data", 1, 4, file_);
    writeU32LE(file_, 0);  // Subchunk2Size (data size) placeholder
}

void WavSink::patchHeaderSizes() {
    uint32_t dataBytes = static_cast<uint32_t>(totalSamplesWritten_ * sizeof(int16_t));
    uint32_t riffChunkSize = 36 + dataBytes;  // 36 = header bytes after this field

    fseek(file_, 4, SEEK_SET);
    writeU32LE(file_, riffChunkSize);

    fseek(file_, 40, SEEK_SET);
    writeU32LE(file_, dataBytes);
}

void WavSink::pushSample(float sample) 
{
    if (!file_)
    {
        return;
    }

    float scaled = sample * gain_;
    if (scaled > 32767.0f) scaled = 32767.0f;
    if (scaled < -32768.0f) scaled = -32768.0f;
    buffer_.push_back(static_cast<int16_t>(scaled));

    if (buffer_.size() >= kFlushThresholdSamples) flush();
}
void WavSink::flush()
{
    if (!file_ || buffer_.empty())
        return;

    fwrite(buffer_.data(), sizeof(int16_t), buffer_.size(), file_);

    totalSamplesWritten_ += buffer_.size();
    buffer_.clear();
}

//void WavSink::flush() 
//{
//    if (!file_ || buffer_.empty())
//    {
//        return;
//    }
//
//    fwrite(buffer_.data(), sizeof(int16_t), buffer_.size(), file_);
//    totalSamplesWritten_ += buffer_.size();
//    buffer_.clear();
//    patchHeaderSizes();
//}
//