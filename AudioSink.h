#ifndef TOASTER_AUDIO_SINK_H
#define TOASTER_AUDIO_SINK_H

#include "ToasterTypes.h"

#include <vector>
#include <string>
#include <fstream>
#include <cstdint>

class AudioSink 
{
public:
    virtual ~AudioSink() = default;

    // Consume one sample, roughly in [-1, 1] before any gain the sink
    // applies internally.
    virtual void pushSample(float sample) = 0;

    // Flush/finalize anything buffered. Safe to call multiple times.
    virtual void flush() = 0;
};

#endif