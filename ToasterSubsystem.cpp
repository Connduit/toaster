#include "ToasterTypes.h"
#include "ToasterSubsystem.h"
#include "Receiver.h"
#include "DspProcessor.h"
#include "Audio.h"
#include "AudioSink.h"

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
    stop();

    delete dspProcessor_;
    delete audio_;
    delete audioSink_;
    //delete fmFilter_;
    delete decimator_;
    delete demodulator_;
    delete dispatcher_;
    delete receiver_;
    if (iFilter_)
    {
        delete iFilter_;
    }
    if (qFilter_)
    {
        delete qFilter_;
    }
    if (audioFilter_)
    {
        delete audioFilter_;
    }
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
    demodulator_ = new Demodulator();
    decimator_ = new Decimator(50);
    //audio_ = new AudioSink(config_.center_freq, config_.other_stuff, etc...);
    audioSink_ = new AudioSink();
    audio_ = new Audio(audioSink_);
    //fmFilter_ = new Filter(2400000.0f, 80000.0f, 101); // old
    //fmFilter_ = new FIRFilter(2400000.0f, 150000.0f, 101);
    //audioFilter_ = new FIRFilter(2400000.0f, 15000.0f, 101);
    iFilter_ = new IIRFilter(80'000.0f, 2'400'000.0f);
    qFilter_ = new IIRFilter(80'000.0f, 2'400'000.0f);
    audioFilter_ = new IIRFilter(15'000.0f, 2'400'000.0f);

    //dspProcessor_ = new DspProcessor(fmFilter_, demodulator_, decimator_, audio_);
    //dspProcessor_ = new DspProcessor(demodulator_, decimator_, audio_);
    dspProcessor_ = new DspProcessor(iFilter_, qFilter_, audioFilter_, demodulator_, decimator_, audio_);
}


void ToasterSubsystem::setupEvents()
{
    std::cout << "ToasterSubsystem::setupEvents()" << std::endl;
    
    // Create the receiver (does NOT call receive() in constructor!)
    //receiver_ = std::make_unique<Receiver>();
    // receiver_->setIQCallback([this](const IQData& iqData)
    // {
    //     // std::cout << "ToasterSubsystem::setupEvents() - setIQCallback" << std::endl;
    //     dispatcher_->dispatch(iqData);
    // });

    // // TODO: change type from 0... 0 is just a filler/default type for now
    // dispatcher_->registerHandler(0, [this](const IQData& iqData)
    // {
    //     // TODO: should probably be processed by DspProcessor and then sent back here to be 
    //     // dispatched again to something like audio or visualizer depending on the command/message type

    //     //auto filteredIQ = fmFilter_->process(iqData);
    //     //auto demodulated = demodulator_->process(filteredIQ);
    //     //auto filteredAudio = audioFilter_->process(demodulated);
    //     //auto audio = decimator_->process(filteredAudio);

    //     auto filteredIQ = fmFilter_->process(iqData);
    //     auto demodulated = demodulator_->process(filteredIQ);
    //     auto audio = decimator_->process(demodulated);

    //     // auto demodulated = demodulator_->process(iqData);
    //     // auto filteredAudio = audioFilter_->process(demodulated);
    //     // auto audio = decimator_->process(filteredAudio);

    //     // auto demodulated = demodulator_->process(iqData);
    //     // auto audio = decimator_->process(demodulated);
    //     audio_->process(audio);
    // });

    receiver_->setIQCallback([this](const IQData &iqData)
    { 
        dispatcher_->dispatch(iqData);
    });

    dispatcher_->registerHandler(0, [this](const IQData &iqData)
    { 
        dspProcessor_->enqueue(iqData);
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
    // processor_ = std::make_unique<DspProcessor>(audio_sample_rate_);
    
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
    dspProcessor_->start();
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
            stop();
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
    //std::cout << "ToasterSubsystem::stop()" << std::endl;
    std::cout << "STOP: entering" << std::endl;

    receiver_->stopAsync();

    std::cout << "STOP: receiver stopped" << std::endl;

    dspProcessor_->stop();

    std::cout << "STOP: DSP stopped" << std::endl;

    audioSink_->finalize();

    std::cout << "STOP: audio finalized" << std::endl;
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