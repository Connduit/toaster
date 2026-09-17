#ifndef TOASTER_DSP_H
#define TOASTER_DSP_H

#include "ToasterTypes.h"
#include "IIRFilter.h"
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

    DspProcessor(std::unique_ptr<Filter> channelFilter, std::unique_ptr<AudioSink> audioSink);

    void processBuffer(const uint8_t* buf, uint32_t len);

    void flush();
    void reset();

private:

    Demodulator demodulator_; // not ptr
    std::unique_ptr<Filter> channelFilter_; // uniqueptr
    Decimator decimator_; // no ptr
    DeemphasisFilter deemphasis_;
    std::unique_ptr<AudioSink> audioSink_; // TODO: uniqueptr
};


#endif