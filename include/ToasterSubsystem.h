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

class ToasterSubsystem
{
public:
    explicit ToasterSubsystem(const ToasterConfig& config);
    ~ToasterSubsystem();
    
    bool start();   // BLOCKS until stop() is called (via signal or manual)
    void stop();    // Stops the receiver and unblocks start()
    
private:
    
    void setupSubcomponents();
    void setupMessaging();
    void setupEvents();
    // void setupTasks();

    const ToasterConfig& config_;

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
    DspProcessorUniquePtr dspProcessor_;
    //std::unique_ptr<Receiver> receiver_;
    // std::unique_ptr<AudioSink> audio_sink_;
    
    
};

#endif
