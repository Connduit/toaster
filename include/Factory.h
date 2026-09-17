#pragma once

#include "Filter.h"
#include "AudioSink.h"
#include "Config.h"

class Factory
{
public:
    static FilterUniquePtr create(const FilterConfig& config);
    static AudioSinkUniquePtr create(const SinkConfig& config);
private:
};
