#include "ToasterTypes.h"
#include "ToasterSubsystem.h"
#include "Receiver.h"
#include "DspProcessor.h"
#include "AudioSink.h"
#include "WavSink.h"
#include "PcmSink.h"
//
#include "Filter.h"
#include "FIRFilter.h"
#include "IIRFilter.h"

#include "FIRBandPassFilter.h"
#include "FIRBandStopFilter.h"
#include "FIRHighPassFilter.h"
#include "FIRLowPassFilter.h"

#include "IIRBandPassFilter.h"
#include "IIRBandStopFilter.h"
#include "IIRHighPassFilter.h"
#include "IIRLowPassFilter.h"
//
#include "Factory.h"
#include <iostream>
#include <csignal>
#include <chrono>
#include <thread>
#include <cstring>

// TODO: these are factories for filters and sink... move them somewhere else
//std::unique_ptr<Filter> makeChannelFilter(const ToasterConfig& config) 
//{
//    if (config.channelFilterType == FilterType::IIR) 
//    {
//        return std::make_unique<IIRLowPassFilter>(config.audioCutoffHz, config.sampleRate_);
//    }
//    return std::make_unique<FIRLowPassFilter>(config.FIRNumTaps, config.audioCutoffHz,
//                                               config.sampleRate_);
//}


ToasterSubsystem::ToasterSubsystem(
    const ToasterConfig &config)
    : config_(config),
      receiver_(),
      console_(),
      stopListener_(console_, [this](){ stop(); })
{
    std::cout << "Custom Config ToasterSubsystem::ToasterSubsystem()" << std::endl;

    //dspProcessor_ = std::make_unique<DspProcessor>(makeChannelFilter(config_), makeSink(config_));
    dspProcessor_ = std::make_unique<DspProcessor>(
                        Factory::create(config_.filterConfig), 
                        Factory::create(config_.sinkConfig));
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
    //stop();

    //delete dspProcessor_;

    //delete audioSink_;

    //delete decimator_;
    //delete demodulator_;

    //delete channelFilter_;

    //delete audioFilter_;

    //delete dispatcher_;
    //delete receiver_;

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
    
    ////receiver_ = new Receiver();
    //dispatcher_ = new Dispatcher();
    //demodulator_ = new Demodulator();
    //decimator_ = new Decimator(50);
    ////audio_ = new AudioSink(config_.center_freq, config_.other_stuff, etc...);
    //audioSink_ = new WavSink();
    ////audio_ = new Audio(audioSink_);
    ////fmFilter_ = new Filter(2400000.0f, 80000.0f, 101); // old
    ////fmFilter_ = new FIRFilter(2400000.0f, 150000.0f, 101);
    ////audioFilter_ = new FIRFilter(2400000.0f, 15000.0f, 101);

    //if (config_.channelFilterType == FilterType::IIR)
    //{
    //    std::cout << "Channel Filter: IIR" << std::endl;
    //    // cutoff, sample rate
    //    channelFilter_ = new IIRLowPassFilter(80'000.0f, 2'400'000.0f);
    //}
    //else if (config_.channelFilterType == FilterType::FIR)
    //{
    //    std::cout << "Channel Filter: FIR" << std::endl;
    //    // numTaps, cutoff, sample rate
    //    channelFilter_ = new FIRLowPassFilter(101, 80'000.0f, 2'400'000.0f);
    //}

    //if (config_.audioFilterType == FilterType::IIR)
    //{
    //    std::cout << "Audio Filter: IIR" << std::endl;
    //    audioFilter_ = new IIRLowPassFilter(15'000.0f, 2'400'000.0f);
    //}
    //else if (config_.audioFilterType == FilterType::FIR)
    //{
    //    std::cout << "Audio Filter: FIR" << std::endl;
    //    audioFilter_ = new FIRLowPassFilter(101, 15'000.0f, 2'400'000.0f);
    //}

    //dspProcessor_ = new DspProcessor(channelFilter_, audioFilter_, demodulator_, decimator_, audioSink_);
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

    // receiver callbacks
    //receiver_->setIQCallback([this](const IQData &iqData)
    //{ 
    //    dispatcher_->dispatch(iqData);
    //});

    //receiver_.setRawSampleCallback([this](const uint8_t* buf, uint32_t len) 
    //{
    //    //dispatcher_->dispatch(buf, len); 
    //    dspProcessor_->processBuffer(buf, len);
    //});


    // dispatcher callbacks
    //dispatcher_->registerHandler(0, [this](const IQData &iqData)
    //{ 
    //    dspProcessor_->enqueue(iqData);
    //});

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

}



bool ToasterSubsystem::start()
{
    // TODO: 
    // initialize default subcomponents if given no custom config
    std::cout << "ToasterSubsystem::start()" << std::endl;
    bool status = false;


    if (!receiver_.open())
    {
        std::cout << "Failed to open RTL-SDR device" << std::endl; 
        return false;
    }

    stopListener_.start();



    // TODO: wait for commands - implement this later
    // For now we will just automatically start receiving
    //dspProcessor_->start(); // TODO: ? 
    //receiver_.startAsync();
    receiver_.startAsync( [this](const uint8_t *buf, uint32_t len)
    { 
        dspProcessor_->processBuffer(buf, len); 
    });
    dspProcessor_->flush();
    // std::string command
    //std::string input;
    //std::cout << "right before while loop " << std::endl;

    //while (std::getline(std::cin, input))
    //{
    //    // TODO: replace with switch/case statement
    //    std::cout << "getline loop..." << std::endl;
    //    if (input == "start")
    //    {

    //    }
    //    else if (input == "stop")
    //    {
    //        //stop();
    //        // status = ...
    //        break;
    //    }
    //}
    //std::cout << "done with while loop" << std::endl;

    return status;
}

void ToasterSubsystem::stop()
{
    receiver_.stop();

    //audioSink_->flush();
}
