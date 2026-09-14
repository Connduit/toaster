#include "Audio.h"
#include "AudioSink.h"

#include <iostream>

Audio::Audio(AudioSink* audioSink)
    : 
    audioSink_(audioSink)
{
}

void Audio::process(const std::vector<float>& audio)
{
    //std::cout << "Audio::process()" << std::endl;

    if (audioSink_)
    {
        audioSink_->process(audio);
    }
}