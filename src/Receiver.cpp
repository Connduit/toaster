// recveiver
// RTL-SDR ---> recveiver ---> TODO:

#include "Receiver.h"

#include <stdexcept>
#include <utility>
#include <iostream>

//Receiver::Receiver(const ReceiverConfig& config)
//    : config_(config), source_(config.deviceIndex) {}

Receiver::Receiver() : source_() 
{
}

bool Receiver::open() {
    if (!source_.open()) return false;

    source_.setSampleRate(1920000);
    source_.setCenterFreq(static_cast<uint32_t>(99900000));

    source_.setAutoGain();
    //if (config_.manualGainTenthsDb < 0) {
    //    source_.setAutoGain();
    //} else {
    //    source_.setManualGain(config_.manualGainTenthsDb);
    //}
    return true;
}

void Receiver::startAsync(RtlSdrDevice::SampleCallback callback) 
{
    source_.startAsync(std::move(callback));
}

void Receiver::stop() { source_.stop(); }

int Receiver::getDeviceCount() { return RtlSdrDevice::getDeviceCount(); }

std::string Receiver::getDeviceName(int index) { return RtlSdrDevice::getDeviceName(index); }
