#pragma once

#include <rtl-sdr.h>

#include <cstdint>
#include <functional>
#include <string>



// Thin RAII wrapper around librtlsdr's C API: opening/closing the device,
// configuring it, and running the async read loop with a C++ callback
// instead of a raw function pointer.
class RtlSdrDevice  // TODO: should inherit from Device.h
{
public:
    // Called with a raw buffer of interleaved uint8 IQ samples (I,Q,I,Q,...)
    // whenever librtlsdr has new data.
    using SampleCallback = std::function<void(const uint8_t* buf, uint32_t len)>;

    explicit RtlSdrDevice(int deviceIndex = 0);
    ~RtlSdrDevice();

    // Non-copyable (owns a device handle), movable not needed for this app.
    RtlSdrDevice(const RtlSdrDevice&) = delete;
    RtlSdrDevice& operator=(const RtlSdrDevice&) = delete;

    bool open();
    void close();

    void setSampleRate(uint32_t sampleRateHz);
    void setCenterFreq(uint32_t freqHz);
    void setAutoGain();
    void setManualGain(int tenthsDb);

    static int getDeviceCount();
    static std::string getDeviceName(int index);

    // Blocks the calling thread, invoking `callback` for each buffer of
    // samples, until stop() is called (e.g. from a signal handler).
    void startAsync(SampleCallback callback, uint32_t bufferLength = 16 * 16384);

    // Safe to call from a signal handler; unblocks startAsync().
    void stop();

private:
    static void rawCallbackTrampoline(unsigned char* buf, uint32_t len, void* ctx);

    int deviceIndex_;
    rtlsdr_dev_t* dev_ = nullptr;
    SampleCallback callback_;
};
