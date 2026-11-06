#include "rf/RtlSdrSource.h"

#include <cstdio>

RtlSdrSource::RtlSdrSource(int deviceIndex) : deviceIndex_(deviceIndex) {}

RtlSdrSource::~RtlSdrSource() { close(); }

int RtlSdrSource::getDeviceCount() { return rtlsdr_get_device_count(); }

std::string RtlSdrSource::getDeviceName(int index) {
    const char* name = rtlsdr_get_device_name(index);
    return name ? std::string(name) : std::string("unknown");
}

bool RtlSdrSource::open() {
    if (dev_) return true;  // already open
    int result = rtlsdr_open(&dev_, deviceIndex_);
    if (result != 0) {
        fprintf(stderr, "Failed to open RTL-SDR device %d (error %d)\n", deviceIndex_, result);
        dev_ = nullptr;
        return false;
    }
    return true;
}

void RtlSdrSource::close() {
    if (dev_) {
        rtlsdr_close(dev_);
        dev_ = nullptr;
    }
}

void RtlSdrSource::setSampleRate(uint32_t sampleRateHz) {
    if (dev_) rtlsdr_set_sample_rate(dev_, sampleRateHz);
}

void RtlSdrSource::setCenterFreq(uint32_t freqHz) {
    if (dev_) rtlsdr_set_center_freq(dev_, freqHz);
}

void RtlSdrSource::setAutoGain() {
    if (dev_) rtlsdr_set_tuner_gain_mode(dev_, 0);
}

void RtlSdrSource::setManualGain(int tenthsDb) {
    if (!dev_) return;
    rtlsdr_set_tuner_gain_mode(dev_, 1);
    rtlsdr_set_tuner_gain(dev_, tenthsDb);
}

void RtlSdrSource::rawCallbackTrampoline(unsigned char* buf, uint32_t len, void* ctx) {
    auto* self = static_cast<RtlSdrSource*>(ctx);
    if (self && self->callback_) {
        self->callback_(buf, len);
    }
}

void RtlSdrSource::startAsync(SampleCallback callback, uint32_t bufferLength) {
    if (!dev_) return;
    callback_ = std::move(callback);
    rtlsdr_reset_buffer(dev_);
    // This call blocks until stop() triggers rtlsdr_cancel_async().
    rtlsdr_read_async(dev_, &RtlSdrSource::rawCallbackTrampoline, this, 0, bufferLength);
}

void RtlSdrSource::stop() {
    if (dev_) rtlsdr_cancel_async(dev_);
}
