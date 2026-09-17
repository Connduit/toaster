#ifndef TOASTER_CONFIG_H
#define TOASTER_CONFIG_H

#include "ToasterTypes.h"
#include <cstdint>

typedef struct ReceiverConfig
{

} ReceiverConfig;

typedef struct SinkConfig
{

} SinkConfig;

typedef struct ToasterConfig
{
        // TODO: SinkConfig
        // TODO: ReceiverConfig

        // FilterType filterType_;
        uint32_t centerFreq_ = 99'900'000;
        uint32_t sampleRate_ = 2'400'000;


        // Audio pipeline (DSP)
        uint32_t audioSampleRateHz = 48000;
        double deemphasisMicroseconds = 75.0;

        // Anti-aliasing filter (applied to the demodulated signal, before
        // decimation)
        FilterType channelFilterType = FilterType::FIR;
        int FIRNumTaps = 50;
        double audioCutoffHz = 15000.0;

        FilterType audioFilterType = FilterType::IIR;

        AudioSinkType audioSinkType_ = AudioSinkType::WAV;
        std::string outputWavPath = "output.wav";
        float outputGain = 12000.0f;

} ToasterConfig;

#endif
