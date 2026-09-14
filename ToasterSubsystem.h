#ifndef TOASTER_SUBSYSTEM_H
#define TOASTER_SUBSYSTEM_H

#include "Config.h"
#include "Audio.h"
#include "DspProcessor.h"
#include "Receiver.h"
#include "Dispatcher.h"
#include "Demodulator.h"
#include "Decimator.h"
//#include "Filter.h"
#include "FIRFilter.h"
#include "IIRFilter.h"

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
    ToasterSubsystem(Config& config);
    ~ToasterSubsystem();
    
    bool start();   // BLOCKS until stop() is called (via signal or manual)
    void stop();    // Stops the receiver and unblocks start()
    
private:
    
    void setupSubcomponents();
    void setupMessaging();
    void setupEvents();
    // void setupTasks();


    void toggleRecv();
    //void run();
    
    Receiver* receiver_;
    Dispatcher* dispatcher_;
    Demodulator* demodulator_;
    Decimator* decimator_;
    AudioSink* audioSink_; 
    ChannelFilter* channelFilter_;
    Filter* iFilter_;
    Filter* qFilter_;
    Filter* audioFilter_;
    //Filter* fmFilter_;
    //Filter* audioFilter_;
    Audio* audio_;
    DspProcessor* dspProcessor_;
    //std::unique_ptr<Receiver> receiver_;
    // std::unique_ptr<DspProcessor> processor_;
    // std::unique_ptr<AudioSink> audio_sink_;
    
    
    // TODO: move to receiver class?
    // double frequency_ = 99.9e6;           // 99.9 MHz
    // uint32_t sample_rate_ = 2400000;       // 2.4 Msps
    // uint32_t audio_sample_rate_ = 48000;   // 48 kHz audio
    // std::string output_filename_ = "output.wav";
    Config config_; // TODO: change to Config& config_
    //Config& config_;
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
