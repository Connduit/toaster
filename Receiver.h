// recveiver
// RTL-SDR ---> recveiver ---> TODO:
#ifndef TOASTER_RECEIVER_H
#define TOASTER_RECEIVER_H

#include "ToasterTypes.h"
#include "RtlSdrDevice.h"

// TODO: in the far far future, be able to handle multiple receiver devices
#include <atomic>
#include <complex>
#include <functional>
#include <thread>
#include <vector>

class Receiver
{
public:
    Receiver();
    //explicit Receiver(const ReceiverConfig &config);

    // Opens and configures the device (sample rate, frequency, gain).
    // Returns false on failure.
    bool open();

    // Streams raw IQ buffers to `callback` until stop() is called.
    // Blocks the calling thread.
    void startAsync(RtlSdrDevice::SampleCallback callback);

    // Safe to call from a signal handler; unblocks startAsync().
    void stop();

    static int getDeviceCount();
    static std::string getDeviceName(int index);


private:
    //ReceiverConfig config_;
    RtlSdrDevice source_; // rename var

    // TODO: Consider replacing IQData with an IQData struct later
    // if we need metadata such as sample rate, center frequency,
    // timestamps, etc.
};

#endif