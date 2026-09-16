// recveiver
// RTL-SDR ---> recveiver ---> TODO:
#ifndef TOASTER_RECEIVER_H
#define TOASTER_RECEIVER_H

#include "ToasterTypes.h"

// TODO: in the far far future, be able to handle multiple receiver devices
#include <rtl-sdr.h>
#include <atomic>
#include <complex>
#include <functional>
#include <thread>
#include <vector>

class Receiver
{
public:
    //using IQData = std::vector<std::complex<float>>;
    using IQCallback = std::function<void(const IQData&)>;
    using RawSampleCallback = std::function<void(const uint8_t *buf, uint32_t len)>;

    Receiver();
    ~Receiver();

    void setIQCallback(IQCallback callback);
    void setRawSampleCallback(RawSampleCallback callback);

    void startAsync();
    void stopAsync();

    bool isReceiving() const;

private:
    static void rtlsdrCallback(
        unsigned char* buffer,
        uint32_t length,
        void* context
    );

    void receive();

    void processRawData( unsigned char* buffer, uint32_t length);

    rtlsdr_dev_t* device_; // = nullptr; // constructor inits device_ to nullptr

    std::thread receiveThread_;
    std::atomic<bool> receiving_;

    IQCallback iqCallback_;
    RawSampleCallback rawSampleCallback_;

    // TODO: Consider replacing IQData with an IQData struct later
    // if we need metadata such as sample rate, center frequency,
    // timestamps, etc.
};

#endif