#ifndef TOASTER_FILTER_H
#define TOASTER_FILTER_H

#include "ToasterTypes.h"

// TODO: put all filters in a "namespace Filter" ? 

// // TODO: change to be a template class if we code to work with any type of filter/data type?
class Filter
{
public:
    virtual ~Filter() = default;
    virtual float process(float input) = 0;
private:
};

// // FIR (finite impulse response)
// // IIR (infinite impulse response)
// 
// /*
// y_n = (x_n + x_(n-1))/2
// where x_n is current sample, x_(n-1) is previous sample, y_n = current sample output
// 
// y_n = ax_n + (1 - a)x_(n-1)
// https://en.wikipedia.org/wiki/Low-pass_filter#Simple_infinite_impulse_response_filter
// y_n = ax_n - (1 - a)x_(n-1)

#endif