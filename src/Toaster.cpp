#include "Toaster.h"


#include <iostream>
#include <csignal>

//Toaster* Toaster::instance_ = nullptr;

Toaster::Toaster() : config_() 
{
}
//Toaster::Toaster() : toasterSubsystem_(std::make_unique<ToasterSubsystem>()),
    //toasterSubsystem_(nullptr),
    //config_()
//{
//}

Toaster::~Toaster()
{
    std::cout << "Toaster::~Toaster()" << std::endl;
    if (toasterSubsystem_)
    {
        delete toasterSubsystem_;
    }

}

void Toaster::createToasterSubsystem()
{
    std::cout << "Toaster::createToasterSubsystem()" << std::endl;
    toasterSubsystem_ = new ToasterSubsystem(config_);
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
    if (toasterSubsystem_) 
    {
        toasterSubsystem_->stop();
    }
}


// void Toaster::loadConfig() {}
