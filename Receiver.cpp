// recveiver
// RTL-SDR ---> recveiver ---> TODO:

#include "Receiver.h"

#include <stdexcept>
#include <utility>
#include <iostream>

constexpr size_t BUFFER_SIZE = 16 * 16384;  // 262,144 bytes per buffer

Receiver::Receiver()
    : device_(nullptr),
      receiving_(false)
{
    int result = rtlsdr_open(&device_, 0);

    // TODO: should we crash just cuz we can't find receiver? ...for now yes
    if (result != 0)
    {
        throw std::runtime_error("Failed to open RTL-SDR device");
    }

    // SDR configuration can go here.
    //
    // rtlsdr_set_center_freq(device_, 95700000);
    // rtlsdr_set_sample_rate(device_, 2400000);
    // rtlsdr_set_tuner_gain_mode(device_, 1);
    // rtlsdr_set_tuner_gain(device_, 300);
}

Receiver::~Receiver()
{
    stopAsync();

    if (device_ != nullptr)
    {
        rtlsdr_close(device_);
        device_ = nullptr;
    }
}

void Receiver::setIQCallback(IQCallback callback)
{
    iqCallback_ = std::move(callback);
}

void Receiver::startAsync()
{
    if (receiving_)
    {
        return;
    }

    receiving_ = true;

    receiveThread_ = std::thread(
        &Receiver::receive,
        this
    );
}

void Receiver::stopAsync()
{
    if (!receiving_)
    {
        return;
    }

    rtlsdr_cancel_async(device_);

    if (receiveThread_.joinable())
    {
        receiveThread_.join();
    }

    receiving_ = false;
}

bool Receiver::isReceiving() const
{
    return receiving_;
}

void Receiver::receive()
{
    std::cout << "Receiver::receive()" << std::endl;

    int result = rtlsdr_reset_buffer(device_);

    result = rtlsdr_read_async(
        device_,
        &Receiver::rtlsdrCallback,
        this,
        0,
        0
    );

    std::cout << "rtlsdr_read_async() returned: " << result << std::endl;

    receiving_ = false;
}

void Receiver::rtlsdrCallback(
    unsigned char* buffer,
    uint32_t length,
    void* context
)
{
    //std::cout << "Receiver::rtlsdrCallback()" << std::endl;
    auto* receiver = static_cast<Receiver*>(context);

    receiver->processRawData(buffer, length);
}

void Receiver::processRawData(
    unsigned char* buffer,
    uint32_t length
)
{
    //std::cout << "Receiver::processRawData()" << std::endl;
    IQData iqData;

    iqData.reserve(length / 2);

    for (uint32_t i = 0; i < length; i += 2)
    {
        const float I =
            (static_cast<float>(buffer[i]) - 127.5f) / 127.5f;

        const float Q =
            (static_cast<float>(buffer[i + 1]) - 127.5f) / 127.5f;

        iqData.emplace_back(I, Q);
    }

    if (iqCallback_)
    {
        iqCallback_(iqData);
    }
}