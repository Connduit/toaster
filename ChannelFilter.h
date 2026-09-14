#ifndef TOASTER_CHANNEL_FILTER_H
#define TOASTER_CHANNEL_FILTER_H

#include "Filter.h"

// TODO: is this name misleading? since it doesn't inherit from Filter as its parent?
class ChannelFilter
{
public:
    ChannelFilter(Filter* iFilter, Filter* qFilter);

    std::complex<float> process(const std::complex<float>& input);

private:
    Filter* iFilter_;
    Filter* qFilter_;
};

#endif