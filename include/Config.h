#ifndef TOASTER_CONFIG_H
#define TOASTER_CONFIG_H

#include "ToasterTypes.h"
#include <cstdint>

typedef struct FilterConfig // TODO: this should probably have a constructor
{
    FilterImplType filterImpl = FilterImplType::FIR;
    FilterType filterType = FilterType::LOWPASS;
    double sampleRateHz = 2'400'000;
    double cutoffHz = 15000.0;
    double lowCutoff = 15000.0;
    double highCutoff = 15000.0;
    int numTaps = 63;
    //double q = ;
    //int order = ;


} FilterConfig;

typedef struct ReceiverConfig
{

} ReceiverConfig;

typedef struct SinkConfig
{
    AudioSinkType audioSinkType = AudioSinkType::WAV;
    std::string outputWavPath = "output.wav";
    float outputGain = 12000.0f;
    uint32_t sampleRateHz = 48000; // correct units?
    int channels = 1;
    // PCM
    //FILE* stream = stdout, 
    // float gain = 1.0f, 
    //size_t reserveSamples = 4096

} SinkConfig;

typedef struct ToasterConfig
{
    // TODO: SinkConfig
    // TODO: ReceiverConfig
    FilterConfig filterConfig;
    SinkConfig sinkConfig;

    // FilterType filterType_;
    uint32_t centerFreq_ = 99'900'000;
    uint32_t sampleRate_ = 2'400'000;


    // Audio pipeline (DSP)
    uint32_t audioSampleRateHz = 48000;
    double deemphasisMicroseconds = 75.0;

    double audioCutoffHz = 15000.0;


} ToasterConfig;

#endif
