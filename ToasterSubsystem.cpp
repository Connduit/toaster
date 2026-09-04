#include "ToasterSubsystem.h"
#include "Receiver.h"
#include "DspProcessor.h"
#include "Audio.h"
#include <iostream>
#include <csignal>
#include <chrono>
#include <thread>

// ============================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================

ToasterSubsystem::ToasterSubsystem()
    : 
	running_(false), 
	initialized_(false)
{
    std::cout << "ToasterSubsystem created" << std::endl;
}

ToasterSubsystem::~ToasterSubsystem()
{
    std::cout << "ToasterSubsystem destroyed" << std::endl;
    shutdown();
}

// ============================================================
// LIFECYCLE
// ============================================================

bool ToasterSubsystem::initialize()
{
    if (initialized_) {
        std::cout << "Already initialized" << std::endl;
        return true;
    }
    
    std::cout << "ToasterSubsystem::initialize()" << std::endl;
    
    // Create all components
    setupComponents();
    
    // Set up the callbacks
    setupCallbacks();
    
    initialized_ = true;
    std::cout << "✅ ToasterSubsystem initialized" << std::endl;
    
    return true;
}

void ToasterSubsystem::shutdown()
{
    if (!initialized_) {
        return;
    }
    
    std::cout << "ToasterSubsystem::shutdown()" << std::endl;
    
    // Make sure we're stopped
    stop();
    
    // Save the WAV file if we have audio
    if (audio_sink_) {
        audio_sink_->finalize();
        std::cout << "  Audio saved: " << output_filename_ << std::endl;
        std::cout << "  Samples: " << audio_sink_->getSampleCount() << std::endl;
        std::cout << "  Duration: " << audio_sink_->getDurationSeconds() << " seconds" << std::endl;
    }
    
    // Release components (smart pointers handle cleanup)
    receiver_.reset();
    processor_.reset();
    audio_sink_.reset();
    
    initialized_ = false;
    std::cout << "ToasterSubsystem shut down" << std::endl;
}

// ============================================================
// CONTROL
// ============================================================

bool ToasterSubsystem::start()
{
    if (!initialized_) {
		initialize();
        std::cerr << "❌ Not initialized! Call initialize() first." << std::endl;
        //return false;
    }
    /*
	else
	{
		initialize();
	}*/
    
    if (running_) {
        std::cout << "Already running" << std::endl;
        return true;
    }
    
    std::cout << "ToasterSubsystem::start()" << std::endl;
    
    // Reset the audio sink for a new recording
    if (audio_sink_) {
        //audio_sink_->reset();
        audio_sink_->setFilename(output_filename_);
        audio_sink_->setSampleRate(audio_sample_rate_);
    }
    
    // Open the receiver
    if (!receiver_->openDevice(0)) {
        std::cerr << "❌ Failed to open SDR device" << std::endl;
        return false;
    }
    
    // Configure the receiver
    receiver_->setCenterFrequency(frequency_);
    receiver_->setSampleRate(sample_rate_);
    receiver_->setAgcMode(true);
    
    // Set running flag
    running_ = true;
    
    // Print status
    std::cout << "📻 Starting receiver..." << std::endl;
    std::cout << "   Frequency: " << frequency_ / 1e6 << " MHz" << std::endl;
    std::cout << "   Sample rate: " << sample_rate_ / 1e6 << " Msps" << std::endl;
    std::cout << "   Output file: " << output_filename_ << std::endl;
    std::cout << "   Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;
    
    // 
    // ⚠️ THIS BLOCKS until stop() is called
    bool result = receiver_->receive();
    
    // We get here when stop() is called (via signal or manual)
    running_ = false;
    
    std::cout << std::endl;
    std::cout << "📻 Receiver stopped" << std::endl;
    std::cout << "   Total samples: " << (audio_sink_ ? audio_sink_->getSampleCount() : 0) << std::endl;
    std::cout << "   Duration: " << (audio_sink_ ? audio_sink_->getDurationSeconds() : 0) << " seconds" << std::endl;
    
    return result;
}

void ToasterSubsystem::stop()
{
    if (!running_) {
        std::cout << "Already stopped" << std::endl;
        return;
    }
    
    std::cout << "ToasterSubsystem::stop() - stopping..." << std::endl;
    
    // Set running_ to false FIRST so the callback knows to stop
    running_ = false;
    
    // Stop the receiver (this cancels rtlsdr_read_async)
    if (receiver_) {
        receiver_->stop();
        std::cout << "  ✅ Receiver stopped" << std::endl;
    }
}

// ============================================================
// STATUS
// ============================================================

size_t ToasterSubsystem::getAudioSampleCount() const
{
    if (audio_sink_) {
        return audio_sink_->getSampleCount();
    }
    return 0;
}

float ToasterSubsystem::getAudioDurationSeconds() const
{
    if (audio_sink_) {
        return audio_sink_->getDurationSeconds();
    }
    return 0.0f;
}

// ============================================================
// CONFIGURATION
// ============================================================

void ToasterSubsystem::setFrequency(double frequency_hz)
{
    frequency_ = frequency_hz;
    if (receiver_ && receiver_->isOpen()) {
        receiver_->setCenterFrequency(frequency_hz);
    }
}

void ToasterSubsystem::setSampleRate(uint32_t sample_rate)
{
    sample_rate_ = sample_rate;
    if (receiver_ && receiver_->isOpen()) {
        receiver_->setSampleRate(sample_rate);
    }
}

void ToasterSubsystem::setOutputFile(const std::string& filename)
{
    output_filename_ = filename;
    if (audio_sink_) {
        audio_sink_->setFilename(filename);
    }
}

void ToasterSubsystem::setAudioSampleRate(uint32_t audio_sample_rate)
{
    audio_sample_rate_ = audio_sample_rate;
    if (audio_sink_) {
        audio_sink_->setSampleRate(audio_sample_rate);
    }
}

// ============================================================
// PRIVATE SETUP METHODS
// ============================================================

void ToasterSubsystem::setupComponents()
{
    std::cout << "ToasterSubsystem::setupComponents()" << std::endl;
    
    // Create the receiver (does NOT call receive() in constructor!)
    receiver_ = std::make_unique<Receiver>();
    std::cout << "  ✅ Receiver created" << std::endl;
    
    // Create the DSP processor
    processor_ = std::make_unique<DspProcessor>(audio_sample_rate_);
    std::cout << "  ✅ DspProcessor created (audio rate: " << audio_sample_rate_ << " Hz)" << std::endl;
    
    // Create the audio sink (saves to file)
    audio_sink_ = std::make_unique<AudioSink>();
    audio_sink_->setFilename(output_filename_);
    audio_sink_->setSampleRate(audio_sample_rate_);
    audio_sink_->setFormat(AudioSink::Format::WAV);
    std::cout << "  ✅ AudioSink created (output: " << output_filename_ << ")" << std::endl;
}

void ToasterSubsystem::setupCallbacks()
{
    std::cout << "ToasterSubsystem::setupCallbacks()" << std::endl;
    
    // ⚠️ CRITICAL: Set the callback BEFORE calling receive()
    receiver_->setOnData([this](const std::vector<std::complex<float>>& iq_samples) {
        // This runs in the SDR callback thread!
        // Check if we're still running
        if (!running_) {
            return;
        }
        
        // 1. Process the IQ samples through DSP
        auto audio = processor_->process(iq_samples);
        
        // 2. Write the audio to the file
        if (audio_sink_) 
        {
            audio_sink_->write(audio);
        }
        
        // 3. Print status every ~1 second
        static size_t last_print = 0;
        if (audio_sink_) {
            size_t count = audio_sink_->getSampleCount();
            if (count - last_print > audio_sample_rate_) {
                last_print = count;
                std::cout << "\r   Recording: " << count / audio_sample_rate_ 
                          << "s   " << std::flush;
            }
        }
    });
    
    std::cout << "  Callback set" << std::endl;
}

// /*
// ClientSubsystem::ClientSubsystem()
// {
// 	// loadConfig?
// 	// enable debugging messages (depending on config?)
// 	setupSubcomponents();
// 	setupMessaging();
// 	// setupEvents();
// 	// setupTasks();
// }
// */
// 
// // ToasterSubsystem::ToasterSubsystem(
// // 		Config& config) : config_(config)
// ToasterSubsystem::ToasterSubsystem()
// {
// 	// do config? 
// 	// enable debugging messages (depending on config?)
// 	setupSubcomponents();
// 	setupMessaging();
// 	setupEvents();
// 	// setupTasks();
// }
// 
// ToasterSubsystem::~ToasterSubsystem()
// {
//     // NOTE: needs to be deleted in correct order
// 
// }
// 
// // TODO: rename function?
// void ToasterSubsystem::setupMessaging()
// {
// 	std::cout << "ToasterSubsystem::setupMessaging()" << std::endl;
// 
// 	// sample rate, cutoff freq, width 
//     //filter_ = new Filter(2400000.0f, 200000.0f, 50000.0f);
// 	// TODO: this is wrong... there should be a filter "factory" here instead, also
// 	// Toaster will definelty have more than one filter it needs to use, so i 
// 	// need to figure out how i want to do that
//     //filter_ = new LowPass(2400000.0f, 200000.0f, 50000.0f);
// 	std::cout << "before recv" << std::endl; 
// 	receiver_ = std::make_unique<Receiver>();
// 	std::cout << "after recv" << std::endl; 
// 	processor_ = std::make_unique<DspProcessor>(48000.0f);
// 	std::cout << "after dsp" << std::endl; 
//     audio_sink_ = std::make_unique<AudioSink>();
// 
// 	std::cout << "about to set filename" << std::endl; 
// 	audio_sink_->setFilename("output.wav");
// 	audio_sink_->setSampleRate(48000);
// 
// }
// 
// void ToasterSubsystem::setupSubcomponents()
// {
// 	std::cout << "ToasterSubsystem::setupSubcomponents()" << std::endl;
// 	
// 	// TODO: ComponentFactory to create all subcomponents used in DspProcessor (DSP)
// 	//serializer_ = ComponentFactory::create(config_.serializerType_);
// 	//encoder_ = ComponentFactory::create(config_.encoderType_);
// 	//encryptor_ = ComponentFactory::create(config_.encryptorType_);
// }
// 
// 
// // TODO: separate into 2 functions... setupCallbacks() and registerEvents()
// void ToasterSubsystem::setupEvents()
// {
// 	std::cout << "ToasterSubsystem::setupEvents()" << std::endl;
// 
// 	//receiver_->setOnData([this](const unsigned char* buffer, uint32_t length, void* ctx)
// 	// receiver_->setOnData([this](const std::vector<std::complex<float>>& iqSamples)
// 	// {
// 
// 	// });
// 
//     // ✅ Set up the callback with debug logging
//     receiver_->setOnData([this](const std::vector<std::complex<float>>& iq_samples) {
//         // This will print EVERY time a buffer is received
//         static int callback_count = 0;
//         callback_count++;
//         
//         if (callback_count % 10 == 0) {
//             std::cout << "📡 Callback #" << callback_count 
//                       << " - samples: " << iq_samples.size() 
//                       << ", running: " << running_ << std::endl;
//         }
//         
//         // Check if we should stop
//         if (!running_) {
//             std::cout << "⏹️ Callback ignored - not running" << std::endl;
//             return;
//         }
//         
//         // 1. Process DSP
//         auto audio = processor_->process(iq_samples);
//         
//         // 2. Save to file
//         audio_sink_->write(audio);
//         
//         // 3. Print status
//         static size_t last_print = 0;
//         auto count = audio_sink_->getSampleCount();
//         if (count - last_print > 48000) { // Every second
//             last_print = count;
//             std::cout << "\rRecording: " << count / 48000 << "s   " << std::flush;
//         }
//     });
// 
//     /*
// 	// transportLayer_ calls back to MessageParser
// 	transportLayer_->setOnMessage([&](const RawByteBuffer& msg)
// 	{
// 			std::cout << "transportLayer_->setOnMessage" << std::endl;
// 			messageHandler_->handle(msg);
// 			//dispatcher_->dispatch(messageTransformer_->transform(msg));
// 	});
// 
// 	// messageParser_ calls back to Dispatcher
// 	messageHandler_->setOnMessage([&](const InternalMessage& msg)
// 	{
// 			std::cout << "dispatch_->setOnMessage" << std::endl;
// 			dispatcher_->dispatch(msg);
// 	});
// 
// 
// 	//////////////////////////////////////
// 	// TODO: controller should have a enum block? 
// 	// that way we can use one overloaded handle function
// 
// 
// 	dispatcher_->registerHandler(MessageType::DEFAULT, [&](const InternalMessage& msg)
// 	{
// 		std::cout << "dispatch_->registerHandler(MessageType::DEFAULT)" << std::endl;
// 		controller_->handleDefault(msg);
// 	});
// 
// 	dispatcher_->registerHandler(MessageType::NONE, [&](const InternalMessage& msg)
// 	{
// 		std::cout << "dispatch_->registerHandler(MessageType::NONE)" << std::endl;
// 		controller_->handleNone(msg);
// 	});
// 
// 	// dispatcher_ executes function associated with MessageType::EXECUTE_COMMAND
// 	dispatcher_->registerHandler(MessageType::EXECUTE_COMMAND, [&](const InternalMessage& msg)
// 	{
// 		std::cout << "dispatch_->registerHandler(MessageType::EXECUTE_COMMAND)" << std::endl;
// 		controller_->handleExecuteCommand(msg);
// 	});
// 
// 	dispatcher_->registerHandler(MessageType::EXECUTE_SHELL_CODE, [&](const InternalMessage& msg)
// 	{
// 		std::cout << "dispatch_->registerHandler(MessageType::EXECUTE_SHELL_CODE)" << std::endl;
// 		controller_->handleExecuteShellcode(msg);
// 	});
// 
// 	dispatcher_->registerHandler(MessageType::SYS_INFO, [&](const InternalMessage& msg)
// 	{
// 		std::cout << "dispatch_->registerHandler(MessageType::SYS_INFO)" << std::endl;
// 		controller_->handleSystemInfo(msg);
// 	});
//     */
// }
// 
// // void ToasterSubsystem::setupTasks() {}
// 
// 
// void ToasterSubsystem::run() // TODO: rename to start??
// {	
// 
// 	// 1. manually resolve apis
// 	//apiManager_->loadAPIs();
// 	/*
// 	TODO:
// 	would i be more effient to find the function/procedure names all at once for a single dll/module so i don't have
// 	to loop through the array of functions every single time?
// 
// 	something like this:
// 	resolve_all_functions(dll_base, hashes, pointers, count);
// 
// 	not sure if this is worth doing for dlls/modules tho
// 
// 	*/
// 	//std::cout << "ClientSubsystem::run()" << std::endl;
// 
// 	// transportLayer_->connect();
// 	// std::thread t([this] { transportLayer_->run(); });
// 	// t.detach();
// 
// 	std::cout << "ToasterSubsystem::run()" << std::endl;
// 	running_ = true;
// 	receiver_->receive();
// }