#ifndef TOASTER_CONFIG_H
#define TOASTER_CONFIG_H

#include "ToasterTypes.h"
#include <cstdint>

typedef struct Config
{
        // TODO: SinkConfig
        // TODO: ReceiverConfig
        Config() :
                //filterType_(FilterType::LOWPASS) // TODO: filter type bit value combo to keep track of all filters that are needed for the configuration? 
                filterType_(FilterType::FIR), // TODO: rename to channelFilterType_ ?

                // center freq - rf we are trying to tune to
                centerFreq_(99'900'000), // TODO: uint32_t should be typedef do my own type that i own
                // sample rate
                sampleRate_(2'400'000) // TODO: uint32_t should be typedef do my own type that i own
                {}

        // TODO: Config member attributes
        // FilterType filterType_;
        FilterType filterType_;
        uint32_t centerFreq_;
        uint32_t sampleRate_;
       

} Config;

#endif
