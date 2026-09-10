#ifndef TOASTER_CONFIG_H
#define TOASTER_CONFIG_H

#include <cstdint>

typedef struct Config
{
        Config() :
                //filterType_(FilterType::LOWPASS) // TODO: filter type bit value combo to keep track of all filters that are needed for the configuration? 

                // center freq - rf we are trying to tune to
                uint32_t centerFreq_(99'900'000); // TODO: uint32_t should be typedef do my own type that i own
                // sample rate
                uint32_t sampleRate_(2'400'000); // TODO: uint32_t should be typedef do my own type that i own
                {}

        // TODO: Config member attributes
        // FilterType filterType_;
        uint32_t centerFreq_;
        uint32_t sampleRate_;
       

} Config;

#endif
