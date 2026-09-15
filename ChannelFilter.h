#ifndef TOASTER_CHANNEL_FILTER_H
#define TOASTER_CHANNEL_FILTER_H

#include "Filter.h"

// Filter recv samples to only look at ones close to the center freq
//class ChannelFilter : public Filter
class ChannelFilter
{
public:
    // TODO: ChannelFilter();
    ChannelFilter(Filter* iFilter, Filter* qFilter);

    //float process(float& input) override;
    //float process(const std::complex<float>& input); 
    std::complex<float> process(const std::complex<float>& input); 

private:
    Filter* iFilter_;
    Filter* qFilter_;
};

#endif