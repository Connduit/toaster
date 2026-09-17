#include "Toaster.h"


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
