#ifndef TOASTER_AUDIO_H
#define TOASTER_AUDIO_H

#include "ToasterTypes.h"

class AudioSink;

class Audio
{
public:
    explicit Audio(AudioSink* audioSink);

    //void process(const IQData& iqData);
    //void process(const std::vector<float>& audio); // TODO: rename audio param to something better?
    void process(float sample);

private:
    AudioSink* audioSink_;
};


#endif