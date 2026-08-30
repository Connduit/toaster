// recveiver
// RTL-SDR ---> recveiver ---> TODO:

#include "Receiver.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include <thread>
#include <chrono>

// ============================================================
// CONSTANTS
// ============================================================

constexpr size_t BUFFER_SIZE = 16 * 16384;  // 262,144 bytes per buffer

// ============================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================

Receiver::Receiver()
    : device_(nullptr)
    , device_open_(false)
    , is_receiving_(false)
{
    std::cout << "Receiver created" << std::endl;
}

Receiver::~Receiver()
{
    std::cout << "Receiver destroyed" << std::endl;
    
    // Make sure we stop receiving
    stop();
    
    // Close the device
    closeDevice();
}

// ============================================================
// DEVICE MANAGEMENT
// ============================================================

bool Receiver::openDevice(int device_index)
{
    std::cout << "Receiver::openDevice(" << device_index << ")" << std::endl;
    
    // Check if already open
    if (device_open_) {
        std::cout << "Device already open" << std::endl;
        return true;
    }
    
    // Check if any devices are available
    int device_count = rtlsdr_get_device_count();
    if (device_count == 0) {
        std::cerr << "❌ No RTL-SDR devices found!" << std::endl;
        return false;
    }
    
    std::cout << "Found " << device_count << " RTL-SDR device(s)" << std::endl;
    
    // Print device info
    for (int i = 0; i < device_count; ++i) {
        char manufacturer[256];
        char product[256];
        char serial[256];
        rtlsdr_get_device_usb_strings(i, manufacturer, product, serial);
        std::cout << "  [" << i << "] " << manufacturer << " " << product 
                  << " (SN: " << serial << ")" << std::endl;
    }
    
    // Open the device
    int result = rtlsdr_open(&device_, device_index);
    if (result < 0) {
        std::cerr << "❌ Failed to open device " << device_index << ": " << result << std::endl;
        return false;
    }
    
    device_open_ = true;
    std::cout << "✅ Device opened successfully" << std::endl;
    
    // Apply default configuration
    setCenterFrequency(center_frequency_);
    setSampleRate(sample_rate_);
    setAgcMode(agc_enabled_);
    
    // Print device capabilities
    std::cout << "📻 Device ready" << std::endl;
    std::cout << "   Frequency: " << center_frequency_ / 1e6 << " MHz" << std::endl;
    std::cout << "   Sample rate: " << sample_rate_ / 1e6 << " Msps" << std::endl;
    std::cout << "   AGC: " << (agc_enabled_ ? "ON" : "OFF") << std::endl;
    
    return true;
}

void Receiver::closeDevice()
{
    if (device_) {
        std::cout << "Closing device..." << std::endl;
        rtlsdr_close(device_);
        device_ = nullptr;
        device_open_ = false;
        std::cout << "✅ Device closed" << std::endl;
    }
}

std::string Receiver::getDeviceName(int index) const
{
    char manufacturer[256] = {0};
    char product[256] = {0};
    char serial[256] = {0};
    
    if (rtlsdr_get_device_usb_strings(index, manufacturer, product, serial) == 0) {
        return std::string(manufacturer) + " " + std::string(product);
    }
    
    return "Unknown Device";
}

// ============================================================
// CONFIGURATION
// ============================================================

bool Receiver::setCenterFrequency(double frequency_hz)
{
    if (!device_open_ || !device_) {
        std::cerr << "❌ Device not open" << std::endl;
        return false;
    }
    
    center_frequency_ = frequency_hz;
    int result = rtlsdr_set_center_freq(device_, static_cast<uint32_t>(frequency_hz));
    if (result < 0) {
        std::cerr << "❌ Failed to set frequency: " << result << std::endl;
        return false;
    }
    
    std::cout << "   Frequency set to: " << frequency_hz / 1e6 << " MHz" << std::endl;
    return true;
}

bool Receiver::setSampleRate(uint32_t sample_rate_hz)
{
    if (!device_open_ || !device_) {
        std::cerr << "❌ Device not open" << std::endl;
        return false;
    }
    
    sample_rate_ = sample_rate_hz;
    int result = rtlsdr_set_sample_rate(device_, sample_rate_hz);
    if (result < 0) {
        std::cerr << "❌ Failed to set sample rate: " << result << std::endl;
        return false;
    }
    
    std::cout << "   Sample rate set to: " << sample_rate_hz / 1e6 << " Msps" << std::endl;
    return true;
}

bool Receiver::setAgcMode(bool enable)
{
    if (!device_open_ || !device_) {
        std::cerr << "❌ Device not open" << std::endl;
        return false;
    }
    
    agc_enabled_ = enable;
    int result = rtlsdr_set_agc_mode(device_, enable ? 1 : 0);
    if (result < 0) {
        std::cerr << "❌ Failed to set AGC mode: " << result << std::endl;
        return false;
    }
    
    std::cout << "   AGC " << (enable ? "enabled" : "disabled") << std::endl;
    return true;
}

bool Receiver::setGain(int gain)
{
    if (!device_open_ || !device_) {
        std::cerr << "❌ Device not open" << std::endl;
        return false;
    }
    
    gain_ = gain;
    
    // Disable AGC when setting manual gain
    rtlsdr_set_agc_mode(device_, 0);
    
    int result = rtlsdr_set_tuner_gain(device_, gain);
    if (result < 0) {
        std::cerr << "❌ Failed to set gain: " << result << std::endl;
        return false;
    }
    
    std::cout << "   Gain set to: " << gain << " dB" << std::endl;
    return true;
}

// ============================================================
// RECEIVE / STOP
// ============================================================

bool Receiver::receive()
{
    std::cout << "Receiver::receive() - START" << std::endl;
    
    // Check if device is open
    if (!device_open_ || !device_) {
        std::cerr << "❌ Device not open" << std::endl;
        return false;
    }
    
    // Check if callback is set
    if (!data_callback_) {
        std::cerr << "❌ No data callback set! Call setOnData() first." << std::endl;
        return false;
    }
    
    // Check if already receiving
    if (is_receiving_) {
        std::cout << "Already receiving" << std::endl;
        return true;
    }
    
    // Clear any old samples in the SDR's buffer
    rtlsdr_reset_buffer(device_);
    
    // Set receiving flag
    is_receiving_ = true;
    
    std::cout << "📡 Calling rtlsdr_read_async (will block until stopped)..." << std::endl;
    std::cout << "   Buffer size: " << BUFFER_SIZE << " bytes" << std::endl;
    
    // ⚠️ THIS BLOCKS until rtlsdr_cancel_async() is called!
    int result = rtlsdr_read_async(
        device_,
        sdr_callback,
        this,           // Pass 'this' as ctx
        0,              // Buffer count (0 = default)
        BUFFER_SIZE
    );
    
    // We only get here when rtlsdr_cancel_async() is called
    is_receiving_ = false;
    
    if (result < 0) {
        std::cerr << "❌ rtlsdr_read_async failed: " << result << std::endl;
        return false;
    }
    
    std::cout << "📡 rtlsdr_read_async returned successfully" << std::endl;
    std::cout << "Receiver::receive() - END" << std::endl;
    
    return true;
}

void Receiver::stop()
{
    std::cout << "Receiver::stop() - START" << std::endl;
    
    // Check if we're actually receiving
    if (!is_receiving_) {
        std::cout << "Receiver::stop() - not receiving, nothing to stop" << std::endl;
        return;
    }
    
    // Check if we have a valid device
    if (!device_) {
        std::cerr << "❌ Receiver::stop() - device is null!" << std::endl;
        is_receiving_ = false;
        return;
    }
    
    // ✅ CRITICAL: Cancel the async read to unblock rtlsdr_read_async()
    std::cout << "🛑 Calling rtlsdr_cancel_async()..." << std::endl;
    rtlsdr_cancel_async(device_);
    
    // The flag will be set to false when rtlsdr_read_async() returns
    // But we can set it here too as a safety measure
    is_receiving_ = false;
    
    std::cout << "🛑 rtlsdr_cancel_async() completed" << std::endl;
    std::cout << "Receiver::stop() - END" << std::endl;
}

// ============================================================
// STATIC CALLBACK (Bridge between C and C++)
// ============================================================

void Receiver::sdr_callback(unsigned char* buf, uint32_t len, void* ctx)
{
    // Static counter for debugging
    static std::atomic<size_t> callback_count{0};
    
    // Print every 1000 callbacks to avoid spamming
    size_t count = callback_count.fetch_add(1) + 1;
    if (count % 1000 == 0) {
        std::cout << "📡 SDR callback #" << count 
                  << " - len: " << len << " bytes" << std::endl;
    }
    
    // Cast the void* back to a Receiver pointer
    Receiver* self = static_cast<Receiver*>(ctx);
    self->processBuffer(buf, len);
}

// ============================================================
// PROCESS BUFFER
// ============================================================

void Receiver::processBuffer(unsigned char* buf, uint32_t len)
{
    // Check if callback exists
    if (!data_callback_) {
        return;
    }
    
    // Static counter for debugging
    static std::atomic<size_t> process_count{0};
    
    // Print every 1000 callbacks to avoid spamming
    size_t count = process_count.fetch_add(1) + 1;
    if (count % 1000 == 0) {
        std::cout << "🔄 processBuffer #" << count 
                  << " - len: " << len << " bytes" << std::endl;
    }
    
    // Convert raw bytes to complex<float> IQ samples
    size_t num_samples = len / 2;
    std::vector<std::complex<float>> iq_samples;
    iq_samples.reserve(num_samples);
    
    for (size_t i = 0; i < len; i += 2) {
        // Convert unsigned char (0-255) to float (-1.0 to 1.0)
        // RTL-SDR outputs centered around 127.5
        float i_sample = (static_cast<float>(buf[i]) - 127.5f) / 128.0f;
        float q_sample = (static_cast<float>(buf[i+1]) - 127.5f) / 128.0f;
        iq_samples.emplace_back(i_sample, q_sample);
    }
    
    // Call the user's callback with the IQ samples
    data_callback_(iq_samples);
}

// #include "Receiver.h"
// #include <cmath>
// #include <iostream>
// 
// constexpr size_t BUFFER_SIZE = 16 * 16384; // how large each chunk of sdr data is
//
// Receiver::Receiver()
//     : 
//     openDevice_(false),
//     device_(nullptr)
// {
//     openDevice();
//     //receive();
// }
// 
// bool Receiver::openDevice()
// {
//     std::cout << "Receiver::openDevice()" << std::endl;
//     if (openDevice_ || rtlsdr_get_device_count() == 0)
//     {
//         std::cout << "Receiver::openDevice() - device already open or no devices found" << std::endl;
//         return true;
//     }
// 
//     if (rtlsdr_open(&device_, 0) != 0)
//     {
//         std::cout << "Receiver::openDevice() - Failed to open RTL-SDR" << std::endl;
//         return true;
//     }
// 
//     rtlsdr_set_center_freq(device_, 99'900'000); // tune to center freq
//     rtlsdr_set_sample_rate(device_, 2'400'000); // set sample rate
// 
//     return false;
// 
// }
// 
// 
// bool Receiver::receive()
// {
//     std::cout << "Receiver::receive()" << std::endl;
//     // clear any old samples in the SDR's buffer
//     rtlsdr_reset_buffer(device_);
// 
//     is_receiving_ = true;
// 
//     std::cout << "about to call rtlsdr_read_async" << std::endl;
//     rtlsdr_read_async(
//         device_,
//         sdr_callback,
//         this,
//         0,
//         BUFFER_SIZE
//     );
//     std::cout << "after call rtlsdr_read_async" << std::endl;
// 
//     return false;
// }
// 
// void Receiver::stop()
// {
//     std::cout << "Receiver::stop() - START" << std::endl;
//     
//     // Check if we're actually receiving
//     if (!is_receiving_) 
//     {
//         std::cout << "Receiver::stop() - not receiving, nothing to stop" << std::endl;
//         return;
//     }
//     
//     // Check if we have a valid device
//     if (!device_) {
//         std::cerr << "Receiver::stop() - device is null!" << std::endl;
//         is_receiving_ = false;
//         return;
//     }
//     
//     // ✅ CRITICAL: Cancel the async read to unblock rtlsdr_read_async()
//     std::cout << "🛑 Calling rtlsdr_cancel_async()..." << std::endl;
//     rtlsdr_cancel_async(device_);
//     
//     // The flag will be set to false when rtlsdr_read_async() returns
//     // But we can set it here too as a safety measure
//     is_receiving_ = false;
//     
//     std::cout << "🛑 rtlsdr_cancel_async() completed" << std::endl;
//     std::cout << "Receiver::stop() - END" << std::endl;
// }
// 
// void Receiver::sdr_callback(unsigned char* buf, uint32_t len, void* ctx) {
//     Receiver* self = static_cast<Receiver*>(ctx);
//     self->processBuffer(buf, len);
// }
// 
// void Receiver::processBuffer(unsigned char* buf, uint32_t len) {
//     // Convert bytes to IQ samples
//     std::vector<std::complex<float>> iq_samples;
//     iq_samples.reserve(len / 2);
//     
//     for (size_t i = 0; i < len; i += 2) {
//         float i_sample = (buf[i] - 127.5f) / 128.0f;
//         float q_sample = (buf[i+1] - 127.5f) / 128.0f;
//         iq_samples.emplace_back(i_sample, q_sample);
//     }
//     
//     // Call user callback
//     if (data_callback_) {
//         data_callback_(iq_samples);
//     }
// }