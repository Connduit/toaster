#ifndef TOASTER_SUBSYSTEM_H
#define TOASTER_SUBSYSTEM_H

#include "Audio.h"
#include "DspProcessor.h"
#include "Receiver.h"
#include "Filter.h"

#include <memory>
#include <atomic>
#include <string>
#include <vector>
#include <complex>

// Forward declarations
class Receiver;
class DspProcessor;
class AudioSink;

/**
 * ToasterSubsystem - Orchestrates the SDR receiver, DSP processing, and audio output
 * 
 * Usage:
 *   ToasterSubsystem subsystem;
 *   subsystem.initialize();
 *   subsystem.setFrequency(99.9e6);
 *   subsystem.setOutputFile("fm_radio.wav");
 *   subsystem.start();  // Blocks until stop() is called
 *   subsystem.shutdown();
 */
class ToasterSubsystem
{
public:
    ToasterSubsystem();
    ~ToasterSubsystem();
    
    // ============================================================
    // LIFECYCLE
    // ============================================================
    
    bool initialize();
    void shutdown();
    
    // ============================================================
    // CONTROL
    // ============================================================
    
    bool start();   // BLOCKS until stop() is called (via signal or manual)
    void stop();    // Stops the receiver and unblocks start()
    
    // ============================================================
    // STATUS
    // ============================================================
    
    bool isRunning() const { return running_; }
    bool isInitialized() const { return initialized_; }
    size_t getAudioSampleCount() const;
    float getAudioDurationSeconds() const;
    
    // ============================================================
    // CONFIGURATION
    // ============================================================
    
    void setFrequency(double frequency_hz);
    void setSampleRate(uint32_t sample_rate);
    void setOutputFile(const std::string& filename);
    void setAudioSampleRate(uint32_t audio_sample_rate);
    
private:
    // ============================================================
    // SETUP METHODS
    // ============================================================
    
    void setupComponents();
    void setupCallbacks();
    
    // ============================================================
    // COMPONENTS
    // ============================================================
    
    std::unique_ptr<Receiver> receiver_;
    std::unique_ptr<DspProcessor> processor_;
    std::unique_ptr<AudioSink> audio_sink_;
    
    // ============================================================
    // STATE
    // ============================================================
    
    std::atomic<bool> running_{false};
    std::atomic<bool> initialized_{false};
    
    // ============================================================
    // CONFIGURATION
    // ============================================================
    
    double frequency_ = 99.9e6;           // 99.9 MHz
    uint32_t sample_rate_ = 2400000;       // 2.4 Msps
    uint32_t audio_sample_rate_ = 48000;   // 48 kHz audio
    std::string output_filename_ = "output.wav";
};

// class ToasterSubsystem
// {
// public:
//     ToasterSubsystem();
//     //ToasterSubsystem(Config& config);
//     ~ToasterSubsystem();
// 
// 	// run/start listening?
// 	bool start();
// 	//void start();
// 	void stop();
// 
// 	// void sendResult(); // TODO: it feels like the wrong place to put this function
// 
// 	// TODO:
// 	// void recvMessage(); 
// 	// void startReceiving(); ???
// 
// private:
// 	void setupMessaging();
// 	void setupEvents();
// 	//void setupTasks();
// 
// 	void setupSubcomponents();
// 	//void setupModules();
// 
//     //Filter* filter_;
// 	// Receiver* receiver_;
// 	// AudioSink* audio_;
// 	// DspProcessor* processor_;
// 	std::unique_ptr<Receiver> receiver_;
//     std::unique_ptr<DspProcessor> processor_;
//     std::unique_ptr<AudioSink> audio_sink_; // TODO: rename
// 	//Config& config_;
// 
// 	bool running_ = false;
// 
// };
// 
// 
#endif