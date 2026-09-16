#ifndef TOASTER_DSP_H
#define TOASTER_DSP_H

#include "ToasterTypes.h"
#include "IIRFilter.h"
#include "AudioSink.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>

class Filter;
class Demodulator;
class Decimator;
class Audio;

class DspProcessor
{
public:
    //DspProcessor( Filter* fmFilter, Demodulator* demodulator, Decimator* decimator, Audio* audio);
    //DspProcessor( Demodulator* demodulator, Decimator* decimator, Audio* audio);
    DspProcessor(
        Filter* channelFilter, // TODO: should be a reference since it was created in toaster subsystem. this would also be a very good time to use a unique_ptr
        Filter* audioFilter, 
        Demodulator* demodulator, 
        Decimator* decimator, 
        AudioSink* audioSink);
        //Audio* audio);

    ~DspProcessor();

    void start();
    void stop();

    void enqueue(const IQData& iqData);
    void processBuffer(const uint8_t* buf, uint32_t len);
    void flush();

private:
    void processLoop();
    void process(const IQData& iqData);

    //Filter* channelFilterI_;
    //Filter* channelFilterQ_;
    Filter* channelFilter_; 
    Filter* audioFilter_;

    //Filter* fmFilter_;
    //Filter* audioFilter_;
    Demodulator* demodulator_;
    Decimator* decimator_;
    //Audio* audio_;
    AudioSink* audioSink_;



    std::queue<IQData> queue_;

    std::mutex mutex_;
    std::condition_variable condition_;

    std::thread workerThread_;

    //std::atomic<bool> running_;
    bool running_;
};


#endif