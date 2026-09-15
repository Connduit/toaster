#include "ChannelFilter.h"

ChannelFilter::ChannelFilter(
    Filter* iFilter,
    Filter* qFilter)
    : 
    iFilter_(iFilter),
    qFilter_(qFilter)
{
}

std::complex<float> ChannelFilter::process(
    const std::complex<float>& input)
{
    const float filteredI = iFilter_->process(input.real());
    const float filteredQ = qFilter_->process(input.imag());

    //return std::complex<float>(filteredI, filteredQ);
    return {filteredI, filteredQ};
}