#include "Toaster.h"


Toaster::Toaster() : config_() {}

// TODO: 
// Toaster::Toaster(ToasterConfig config) : config_(config) {}

Toaster::~Toaster()
{
    if (toasterSubsystem_)
    {
        delete toasterSubsystem_;
    }
}

void Toaster::createToasterSubsystem()
{
    toasterSubsystem_ = new ToasterSubsystem(config_);
}

void Toaster::startToasterSubsystem()
{
    if (toasterSubsystem_) 
	{
        toasterSubsystem_->start();
    }
}

// void Toaster::loadConfig() {}
