#include "DspProcessor.h"

#include "Filter.h"
#include "Demodulator.h"
#include "Decimator.h"

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
//DspProcessor::DspProcessor(
//    Filter* channelFilter,
//    Filter* audioFilter,
//    Demodulator* demodulator,
//    Decimator* decimator,
//    AudioSink* audioSink)
//    : 
//    channelFilter_(channelFilter),
//    audioFilter_(audioFilter),
//    demodulator_(demodulator),
//    decimator_(decimator),
//    audioSink_(audioSink)
//{
//}

DspProcessor::DspProcessor(
    std::unique_ptr<Filter> channelFilter,
    std::unique_ptr<AudioSink> audioSink)
    : 
    channelFilter_(std::move(channelFilter)),
    decimator_(static_cast<int>(1920000 / 48000)),
    deemphasis_(48000, 75.0),
    audioSink_(std::move(audioSink)) 
{}


// TODO: this is only for processing for audio
void DspProcessor::processBuffer(const uint8_t* buf, uint32_t len)
{
    for (uint32_t n = 0; n + 1 < len; n += 2)
    {
        float i = (static_cast<float>(buf[n]) - 127.5f) / 127.5f;
        float q = (static_cast<float>(buf[n + 1]) - 127.5f) / 127.5f;

        float demodulated = demodulator_.process(i, q); // merges i and q into a single number
        float filtered = channelFilter_->process(demodulated);

        float decimated;
        if (decimator_.push(filtered, decimated))
        {
            float deemphasized = deemphasis_.process(decimated); // audio filter?
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

void DspProcessor::reset() 
{
    //demodulator_.reset();
    //channelFilter_->reset();
    //decimator_.reset();
    //deemphasis_.reset();
}