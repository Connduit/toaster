#ifndef TOASTER_TYPES_H
#define TOASTER_TYPES_H

#include <complex>
#include <vector>

enum class FilterType
{
    IIR,
    FIR
};

typedef std::vector<std::complex<float>> IQData; // TODO: IQData should just be std::complex<float> ??

typedef std::vector<float> AudioData;

// TODO: enums for where we want to send iqdata (audio, visual, etc)

#endif