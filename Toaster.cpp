#include "Toaster.h"

#include <iostream>

#include "Toaster.h"
#include <iostream>
#include <csignal>

Toaster* Toaster::instance_ = nullptr;

Toaster::Toaster()
    : toasterSubsystem_(nullptr)
{
    std::cout << "Toaster created" << std::endl;
    instance_ = this; 
    
    std::signal(SIGINT, signalHandler);
}

Toaster::~Toaster()
{
    std::cout << "Toaster destroyed" << std::endl;
    delete toasterSubsystem_;
    toasterSubsystem_ = nullptr;
    instance_ = nullptr;
}

void Toaster::createToasterSubsystem()
{
    std::cout << "Toaster::createToasterSubsystem()" << std::endl;
    toasterSubsystem_ = new ToasterSubsystem();
}

void Toaster::startToasterSubsystem()
{
    std::cout << "Toaster::startToasterSubsystem()" << std::endl;
    if (toasterSubsystem_) 
	{
        toasterSubsystem_->start();
    }
}

void Toaster::stopToasterSubsystem()
{
    std::cout << "Toaster::stopToasterSubsystem()" << std::endl;
    if (toasterSubsystem_) {
        toasterSubsystem_->stop();
    }
}

// ✅ Static signal handler - calls stop on the instance
void Toaster::signalHandler(int sig)
{
    if (sig == SIGINT) {
        std::cout << "\n🛑 Ctrl+C received!" << std::endl;
        if (instance_) {
            instance_->stopToasterSubsystem();
        }
    }
}


// void Toaster::loadConfig() {}
