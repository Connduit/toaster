#ifndef TOASTER_DECIMATOR_H
#define TOASTER_DECIMATOR_H

#include "ToasterTypes.h"

class Decimator
{
public:

    explicit Decimator(int factor);

    AudioData process(const AudioData& input);
    bool processSample(float input, float &output);

private:
    int factor_;
    int counter_;

    // Filter state
    //float previousSample_;
};

#endif