#include "DspProcessor.h"

#include "Filter.h"
#include "Demodulator.h"
#include "Decimator.h"
#include "Audio.h"

#include <utility>
#include <iostream>

//DspProcessor::DspProcessor(
//    Filter* fmFilter,
//    Demodulator* demodulator,
//    Decimator* decimator,
//    Audio* audio
//)
//    : fmFilter_(fmFilter),
//      demodulator_(demodulator),
//      decimator_(decimator),
//      audio_(audio),
//      running_(false)
//{
//}


// TODO: dsp should take a dictionary of filters? map<enum FilterType, Filter>
DspProcessor::DspProcessor(
    Filter* channelFilter,
    Filter* audioFilter,
    Demodulator* demodulator,
    Decimator* decimator,
    AudioSink* audioSink)
    : 
    channelFilter_(channelFilter),
    audioFilter_(audioFilter),
    demodulator_(demodulator),
    decimator_(decimator),
    audioSink_(audioSink),
    running_(false)
{
}
// DspProcessor::DspProcessor(
//     Demodulator* demodulator,
//     Decimator* decimator,
//     Audio* audio)
//     : 
//     channelFilterI_(80'000.0f, 2'400'000.0f),
//     channelFilterQ_(80'000.0f, 2'400'000.0f),
//     audioFilter_(15'000.0f, 2'400'000.0f),
//     demodulator_(demodulator),
//     decimator_(decimator),
//     audio_(audio),
//     running_(false)
// {
// }

DspProcessor::~DspProcessor()
{
    stop();
}

void DspProcessor::start()
{
    std::cout << "dsp proc :: start()\n";
    if (running_)
        return;

    running_ = true;

    //workerThread_ = std::thread(&DspProcessor::processLoop, this);
}

void DspProcessor::stop()
{
    if (!running_)
        return;

    {
        //std::lock_guard<std::mutex> lock(mutex_);

        running_ = false;

        // Discard any IQ data that has not been processed yet.
        //std::queue<IQData> empty;
        //queue_.swap(empty);
    }

    //condition_.notify_one();

    //if (workerThread_.joinable())
    //    workerThread_.join();
}

void DspProcessor::enqueue(const IQData& iqData)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);

        queue_.push(iqData);
    }

    condition_.notify_one();
}

// TODO: resource manager / queue logic should be its own class
// when it gets more complex?
void DspProcessor::processLoop()
{
    std::cout << "DspProcessor::processLoop() started" << std::endl;

    while (true)
    {
        IQData iqData;

        {
            std::unique_lock<std::mutex> lock(mutex_);

            condition_.wait(lock, [this]
            {
                return !queue_.empty() || !running_;
            });

            if (!running_)
                break;

            iqData = std::move(queue_.front());
            queue_.pop();
        }

        process(iqData);
    }

    std::cout << "DspProcessor::processLoop() finished" << std::endl;
}

void DspProcessor::process(const IQData& iqData)
{
    std::cout << "DspProcessor::process(const IQData& iqData)\n";
//     auto filteredIQ =
//         fmFilter_->process(iqData); // TODO: fmFilter_ should be renamed/changed to whatever the incoming filter is?
// 
//     auto demodulated =
//         demodulator_->process(filteredIQ);
// 
//     auto audio =
//         decimator_->process(demodulated);
// 
//     audio_->process(audio);
}

// TODO: Note how this function performs computations on a sample by sample basis, rather than putting everything
// in a vector then passing it around
// void DspProcessor::process(const IQData& iqData)
// {
//     AudioData audio;
// 
//     audio.reserve(iqData.size() / 50 + 1);
// 
//     for (const auto& sample : iqData)
//     {
//         auto filteredIQ = channelFilter_->process(sample);
// 
//         auto demodulated = demodulator_->processSample(filteredIQ);
// 
//         auto filteredAudio = audioFilter_->process(demodulated);
// 
//         float decimatedSample;
// 
//         if (decimator_->processSample( filteredAudio, decimatedSample))
//         {
//             audio.push_back(decimatedSample);
//         }
// 
// 
//         // -------------------------
//         // Channel filter
//         // -------------------------
// 
//         // const float filteredI = channelFilterI_->process(sample.real());
//         // const float filteredQ = channelFilterQ_->process(sample.imag());
// 
//         // const std::complex<float> filteredIQ(filteredI, filteredQ);
// 
//         // // -------------------------
//         // // FM demodulation
//         // // -------------------------
// 
//         // const float demodulated =
//         //     demodulator_->processSample(filteredIQ);
// 
//         // // -------------------------
//         // // Audio filter
//         // // -------------------------
// 
//         // const float filteredAudio = audioFilter_->process(demodulated);
// 
//         // // -------------------------
//         // // Decimation
//         // // -------------------------
// 
//         // float decimatedSample;
// 
//         // if (decimator_->processSample(filteredAudio, decimatedSample))
//         // {
//         //     audio.push_back(decimatedSample);
//         // }
//     }
// 
//     // Send the entire resulting audio buffer
//     // to Audio at once.
//     if (!audio.empty())
//     {
//         audio_->process(audio);
//     }
// }

//void DspProcessor::process(const IQData& iqData)
//{
//    AudioData audio;
//
//    audio.reserve(iqData.size() / 50 + 1);
//
//    for (const auto& sample : iqData)
//    {
//        const auto filteredIQ = channelFilter_->process(sample); // float i and float q
//
//        const float demodulated = demodulator_->processSample(filteredIQ);
//
//        const float filteredAudio = audioFilter_->process(demodulated);
//
//        float decimatedSample;
//
//        if (decimator_->processSample(filteredAudio, decimatedSample))
//        {
//            audio.push_back(decimatedSample);
//        }
//    }
//
//    if (!audio.empty())
//    {
//        audio_->process(audio);
//    }
//}


void DspProcessor::processBuffer(const uint8_t* buf, uint32_t len)
{
    for (uint32_t n = 0; n + 1 < len; n += 2)
    {
        float i = (static_cast<float>(buf[n]) - 127.5f) / 127.5f;
        float q = (static_cast<float>(buf[n + 1]) - 127.5f) / 127.5f;

        float demodulated = demodulator_->process(i, q); // merges i and q into a single number
        float filtered = channelFilter_->process(demodulated);

        float decimated;
        if (decimator_->push(filtered, decimated))
        {
            float deemphasized = audioFilter_->process(decimated); // audio filter?
            audioSink_->pushSample(deemphasized);
            //float deemphasized = deemphasis_.process(decimated); // audio filter?
            //sink_->pushSample(deemphasized);
        }
    }
    audioSink_->flush();
}

void DspProcessor::flush()
{
    audioSink_->flush();
}