// recveiver
// RTL-SDR ---> recveiver ---> TODO:
#ifndef TOASTER_RECEIVER_H
#define TOASTER_RECEIVER_H

// TODO: in the far far future, be able to handle multiple receiver devices
#include <rtl-sdr.h>
#include <functional>
#include <vector>
#include <complex>
#include <atomic>

/**
 * Receiver - Wraps the RTL-SDR library for receiving radio signals
 * 
 * Usage:
 *   Receiver receiver;
 *   receiver.setOnData([](const std::vector<std::complex<float>>& iq) {
 *       // Process IQ samples
 *   });
 *   receiver.openDevice(0);
 *   receiver.setCenterFrequency(99.9e6);
 *   receiver.receive();  // Blocks until stop() is called
 */
class Receiver
{
public:
    // Callback type for IQ data
    using IqCallback = std::function<void(const std::vector<std::complex<float>>&)>;
    
    Receiver();
    ~Receiver();
    
    // ============================================================
    // DEVICE MANAGEMENT
    // ============================================================
    
    bool openDevice(int device_index = 0);
    void closeDevice();
    
    // ============================================================
    // CONFIGURATION
    // ============================================================
    
    bool setCenterFrequency(double frequency_hz);
    bool setSampleRate(uint32_t sample_rate_hz);
    bool setAgcMode(bool enable);
    bool setGain(int gain);
    
    // ============================================================
    // CONTROL
    // ============================================================
    
    bool receive();   // BLOCKS until stop() is called
    void stop();      // Cancels the blocking receive
    
    // ============================================================
    // CALLBACK
    // ============================================================
    
    void setOnData(IqCallback cb) { data_callback_ = std::move(cb); }
    
    // ============================================================
    // STATUS
    // ============================================================
    
    bool isOpen() const { return device_open_; }
    bool isReceiving() const { return is_receiving_; }
    int getDeviceCount() const { return rtlsdr_get_device_count(); }
    std::string getDeviceName(int index) const;
    
private:
    // ============================================================
    // STATIC CALLBACK (Bridge between C and C++)
    // ============================================================
    
    static void sdr_callback(unsigned char* buf, uint32_t len, void* ctx);
    void processBuffer(unsigned char* buf, uint32_t len);
    
    // ============================================================
    // MEMBERS
    // ============================================================
    
    // RTL-SDR device handle
    rtlsdr_dev_t* device_ = nullptr;
    
    // State
    bool device_open_ = false;
    bool is_receiving_ = false;
    
    // Configuration
    double center_frequency_ = 99.9e6;    // 99.9 MHz
    uint32_t sample_rate_ = 2400000;       // 2.4 Msps
    bool agc_enabled_ = true;
    int gain_ = 0;
    
    // User callback
    IqCallback data_callback_;
};

// #include <rtl-sdr.h>
// 
// #include <complex>
// #include <functional>
// #include <vector>
// 
// class Receiver
// {
// public:
//     //using Callback = std::function<void(const std::vector<std::complex<float>>&)>;
//     using IqCallback = std::function<void(const std::vector<std::complex<float>> &)>;
// 
//     Receiver();
//     //Receiver(device) : device_(device);
//     //~Receiver();
// 
//     // bool connect();
//     bool receive(); // start recveing
//     void stop();
//     // setCenterFreq;
//     // setSampleRate;
//     // setAgcMode; // auto gain mode
// 
//     // start
//     // stop
// 
// 
// 
//     bool openDevice();
//     void setOnData(IqCallback cb) { data_callback_ = std::move(cb); }
//     //void setOnData(rtlsdr_read_async_cb_t cb) { receiveCallback_ = std::move(cb); }
// 
// 
//     // isOpen //device is open
//     // isReceving
// private:
//     static void sdr_callback(unsigned char *buf, uint32_t len, void *ctx);
//     void processBuffer(unsigned char* buf, uint32_t len);
//     //rtlsdr_read_async_cb_t callback(unsigned char* buf)
//     //rtlsdr_read_async_cb_t receiveCallback_;
//     IqCallback data_callback_;
//     bool is_receiving_ = false;
//     // float centerFreq_; 
//     // float sampleRate_;
// 
//     bool openDevice_;
//     rtlsdr_dev_t* device_;
// };
#endif