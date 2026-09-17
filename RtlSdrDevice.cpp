#include "RtlSdrDevice.h"

#include <cstdio>
#include <iostream>

RtlSdrDevice::RtlSdrDevice(int deviceIndex) : deviceIndex_(deviceIndex) 
{
}

RtlSdrDevice::~RtlSdrDevice() { close(); }

int RtlSdrDevice::getDeviceCount() { return rtlsdr_get_device_count(); }

std::string RtlSdrDevice::getDeviceName(int index) {
    const char* name = rtlsdr_get_device_name(index);
    return name ? std::string(name) : std::string("unknown");
}

bool RtlSdrDevice::open() {
    if (dev_) return true;  // already open
    int result = rtlsdr_open(&dev_, deviceIndex_);
    if (result != 0) {
        fprintf(stderr, "Failed to open RTL-SDR device %d (error %d)\n", deviceIndex_, result);
        dev_ = nullptr;
        return false;
    }
    return true;
}

void RtlSdrDevice::close() {
    if (dev_) {
        rtlsdr_close(dev_);
        dev_ = nullptr;
    }
}

void RtlSdrDevice::setSampleRate(uint32_t sampleRateHz) {
    if (dev_) rtlsdr_set_sample_rate(dev_, sampleRateHz);
}

void RtlSdrDevice::setCenterFreq(uint32_t freqHz) {
    if (dev_) rtlsdr_set_center_freq(dev_, freqHz);
}

void RtlSdrDevice::setAutoGain() {
    if (dev_) rtlsdr_set_tuner_gain_mode(dev_, 0);
}

void RtlSdrDevice::setManualGain(int tenthsDb) {
    if (!dev_) return;
    rtlsdr_set_tuner_gain_mode(dev_, 1);
    rtlsdr_set_tuner_gain(dev_, tenthsDb);
}

void RtlSdrDevice::rawCallbackTrampoline(
    unsigned char* buf,
    uint32_t len,
    void* ctx)
{
    auto* self = static_cast<RtlSdrDevice*>(ctx);

    if (self && self->callback_)
    {
        self->callback_(buf, len);
    }
}

//void RtlSdrDevice::rawCallbackTrampoline(unsigned char* buf, uint32_t len, void* ctx) {
//    auto* self = static_cast<RtlSdrDevice*>(ctx);
//    if (self && self->callback_) {
//        self->callback_(buf, len);
//    }
//}

void RtlSdrDevice::startAsync(SampleCallback callback, uint32_t bufferLength)
{
    if (!dev_) {
        return;
    }

    callback_ = std::move(callback);
    rtlsdr_reset_buffer(dev_);

    int result = rtlsdr_read_async(
        dev_,
        &RtlSdrDevice::rawCallbackTrampoline,
        this,
        0,
        bufferLength);

}

void RtlSdrDevice::stop()
{
    if (!dev_) {
        return;
    }

    int result = rtlsdr_cancel_async(dev_);
}

//void RtlSdrDevice::startAsync(SampleCallback callback, uint32_t bufferLength) {
//    if (!dev_) return;
//    callback_ = std::move(callback);
//    rtlsdr_reset_buffer(dev_);
//    // This call blocks until stop() triggers rtlsdr_cancel_async().
//    rtlsdr_read_async(dev_, &RtlSdrDevice::rawCallbackTrampoline, this, 0, bufferLength);
//}
//
//void RtlSdrDevice::stop() 
//{
//    if (dev_) 
//    {
//        rtlsdr_cancel_async(dev_);
//    }
//}
//