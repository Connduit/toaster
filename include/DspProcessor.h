#ifndef TOASTER_DSP_H
#define TOASTER_DSP_H

#include "ToasterTypes.h"
#include "Filter.h"
#include "AudioSink.h"
#include "Deemphasis.h"
#include "Demodulator.h"
#include "Decimator.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>

class DspProcessor
{
public:
    DspProcessor(FilterUniquePtr channelFilter, AudioSinkUniquePtr audioSink);

    void processBuffer(const uint8_t* buf, uint32_t len);

    void flush();
    void reset();

private:

    Demodulator demodulator_;
    FilterUniquePtr channelFilter_;
    Decimator decimator_;
    DeemphasisFilter deemphasis_;
    AudioSinkUniquePtr audioSink_;
};

typedef std::unique_ptr<DspProcessor> DspProcessorUniquePtr;

#endif