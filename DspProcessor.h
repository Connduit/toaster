#ifndef TOASTER_DSP_H
#define TOASTER_DSP_H

#include "ToasterTypes.h"
#include "LPF.h"

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
    DspProcessor( Demodulator* demodulator, Decimator* decimator, Audio* audio);

    ~DspProcessor();

    void start();
    void stop();

    void enqueue(const IQData& iqData);

private:
    void processLoop();
    void process(const IQData& iqData);

    Filter* fmFilter_;
    Demodulator* demodulator_;
    Decimator* decimator_;
    Audio* audio_;


    LPF channelFilterI_;
    LPF channelFilterQ_;
    LPF audioFilter_;


    std::queue<IQData> queue_;

    std::mutex mutex_;
    std::condition_variable condition_;

    std::thread workerThread_;

    std::atomic<bool> running_;
};


#endif