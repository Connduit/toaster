#include "ToasterTypes.h"
#include "ToasterSubsystem.h"
#include "Receiver.h"
#include "DspProcessor.h"
#include "Audio.h"
#include <iostream>
#include <csignal>
#include <chrono>
#include <thread>
#include <cstring>


ToasterSubsystem::ToasterSubsystem() : config_()
{
    std::cout << "Default ToasterSubsystem::ToasterSubsystem()" << std::endl;
    setupSubcomponents();
    setupMessaging();
    setupEvents();
    //setupTasks();
}

ToasterSubsystem::ToasterSubsystem(
    Config& config) : config_(config)
{
    std::cout << "Custom Config ToasterSubsystem::ToasterSubsystem()" << std::endl;
    // TODO: can i call these in the contructor or will that 
    // mess with the async callback? 
    setupSubcomponents();
    setupMessaging();
    setupEvents();
    // setupTasks();

    // TODO: rtlsdr_read_async either needs an std::atomic<bool> callback flag or
    // someone needs to explicitly call it after the contructor is done
}

ToasterSubsystem::~ToasterSubsystem()
{
    std::cout << "ToasterSubsystem::~ToasterSubsystem()" << std::endl;
}


// factory stuff
void ToasterSubsystem::setupSubcomponents()
{
    std::cout << "ToasterSubsystem::setupSubcomponents()" << std::endl;
}

// This is where classes components are constructed 
void ToasterSubsystem::setupMessaging()
{
    std::cout << "ToasterSubsystem::setupMessaging()" << std::endl;
    //receiver_->setOnData([this](const std::vector<std::complex<float>>& iq_samples) {});
    

    //receiver_ = new Receiver(config_.center_freq, config_.other_stuff, etc...);
    receiver_ = new Receiver();
    dispatcher_ = new Dispatcher();
    //audio_ = new AudioSink(config_.center_freq, config_.other_stuff, etc...);
    audio_ = new AudioSink();

    
}


void ToasterSubsystem::setupEvents()
{
    std::cout << "ToasterSubsystem::setupEvents()" << std::endl;
    
    // Create the receiver (does NOT call receive() in constructor!)
    //receiver_ = std::make_unique<Receiver>();
    receiver_->setIQCallback([this](const IQData& iqData)
    {
        // std::cout << "ToasterSubsystem::setupEvents() - setIQCallback" << std::endl;
        dispatcher_->dispatch(iqData);
    });

    // TODO: change type from 0... 0 is just a filler/default type for now
    dispatcher_->registerHandler(0, [this](const IQData& iqData)
    {
        audio_->process(iqData);
    });

//////////////////////
// 	// messageParser_ calls back to Dispatcher
// 	messageHandler_->setOnMessage([&](const InternalMessage& msg)
// 	{
// 			std::cout << "dispatch_->setOnMessage" << std::endl;
// 			dispatcher_->dispatch(msg);
// 	});

// 	dispatcher_->registerHandler(MessageType::DEFAULT, [&](const InternalMessage& msg)
// 	{
// 		std::cout << "dispatch_->registerHandler(MessageType::DEFAULT)" << std::endl;
// 		controller_->handleDefault(msg);
// 	});

//////////////////////
    // Create the DSP processor
    //processor_ = std::make_unique<DspProcessor>(audio_sample_rate_);
    
    // Create the audio sink (saves to file)
    // audio_sink_ = std::make_unique<AudioSink>();
    // audio_sink_->setFilename(output_filename_);
    // audio_sink_->setSampleRate(audio_sample_rate_);
    // audio_sink_->setFormat(AudioSink::Format::WAV);
}

bool ToasterSubsystem::start()
{
    // TODO: 
    // initialize default subcomponents if given no custom config
    std::cout << "ToasterSubsystem::start()" << std::endl;

    bool status = false;


    // TODO: wait for commands - implement this later
    // For now we will just automatically start receiving
    receiver_->startAsync();

    // std::string command
    std::string input;

    while (std::getline(std::cin, input))
    {
        // TODO: replace with switch/case statement
        if (input == " ")
        {
            toggleRecv();
        }
        else if (input == "start")
        {

        }
        else if (input == "stop")
        {
            // status = ...
            break;
        }
        else if (input == "pause")
        {
            toggleRecv();
        }
    }

    return status;
}

void ToasterSubsystem::stop()
{
    std::cout << "ToasterSubsystem::stop()" << std::endl;
}


// TODO: this should be handled by a dispatcher class?
void ToasterSubsystem::toggleRecv()
{
    std::cout << "ToasterSubsystem::toggleRecv()" << std::endl;
    if (receiver_->isReceiving())
    {
        std::cout << "\tstopAsync" << std::endl;
        receiver_->stopAsync();
    }
    else
    {
        std::cout << "\tstartAsync" << std::endl;
        receiver_->startAsync();
    }
}


// void ToasterSubsystem::run()
// {
//     receiver_->startAsync();
// 
//     char input;
// 
//     while (std::cin.get(input))
//     {
//         if (input == ' ')
//         {
//             toggleRecv();
//         }
//     }
// }