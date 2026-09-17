#ifndef TOASTER_SUBSYSTEM_H
#define TOASTER_SUBSYSTEM_H

#include "Config.h"
#include "DspProcessor.h"
#include "Receiver.h"
#include "Dispatcher.h"
#include "Demodulator.h"
#include "Decimator.h"
//#include "Filter.h"
#include "FIRFilter.h"
#include "IIRFilter.h"
#include "Console.h"
#include "ConsoleStopListener.h"

#include <memory>
#include <atomic>
#include <string>
#include <vector>
#include <complex>



// Forward declarations
class Receiver;
class Dispatcher;
class Demodulator;
class Decimator;
class AudioSink;
class DspProcessor;

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
    explicit ToasterSubsystem(const ToasterConfig& config);
    ~ToasterSubsystem();
    
    // TODO: rename start to run?
    bool start();   // BLOCKS until stop() is called (via signal or manual)
    void stop();    // Stops the receiver and unblocks start()
    
private:
    
    void setupSubcomponents();
    void setupMessaging();
    void setupEvents();
    // void setupTasks();

    ToasterConfig config_; // TODO: change to Config& config_

    //Receiver* receiver_;
    Receiver receiver_;

    Console console_;
    ConsoleStopListener stopListener_;

    //Dispatcher* dispatcher_;

    // first filter - used for reducing out-of-band signals and to prevent aliasing
    //Filter* channelFilter_;


    //Filter* audioFilter_;
    //Demodulator* demodulator_;
    //Decimator* decimator_;
    //AudioSink* audioSink_; 
    //DspProcessor dspProcessor_;
    std::unique_ptr<DspProcessor> dspProcessor_;
    //std::unique_ptr<Receiver> receiver_;
    // std::unique_ptr<DspProcessor> processor_;
    // std::unique_ptr<AudioSink> audio_sink_;
    
    
    // TODO: move to receiver class?
    // double frequency_ = 99.9e6;           // 99.9 MHz
    // uint32_t sample_rate_ = 2400000;       // 2.4 Msps
    // uint32_t audio_sample_rate_ = 48000;   // 48 kHz audio
    // std::string output_filename_ = "output.wav";
};

#endif
