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
    //DspProcessor( Filter* fmFilter, Demodulator* demodulator, Decimator* decimator, Audio* audio);
    //DspProcessor( Demodulator* demodulator, Decimator* decimator, Audio* audio);
    //DspProcessor(
    //    Filter* channelFilter, // TODO: should be a reference since it was created in toaster subsystem. this would also be a very good time to use a unique_ptr
    //    Filter* audioFilter, 
    //    Demodulator* demodulator, 
    //    Decimator* decimator, 
    //    AudioSink* audioSink);
    //    //Audio* audio);

    DspProcessor(FilterUniquePtr channelFilter, AudioSinkUniquePtr audioSink);

    void processBuffer(const uint8_t* buf, uint32_t len);

    void flush();
    void reset();

private:

    Demodulator demodulator_; // not ptr
    FilterUniquePtr channelFilter_; // uniqueptr
    Decimator decimator_; // no ptr
    DeemphasisFilter deemphasis_;
    AudioSinkUniquePtr audioSink_; // TODO: uniqueptr
};


#endif