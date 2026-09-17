#ifndef TOASTER_H
#define TOASTER_H

#include "ToasterSubsystem.h"
#include "Config.h"

class Toaster
{
public:
    Toaster(); 
    ~Toaster();
    void createToasterSubsystem();
    void startToasterSubsystem();
private:
    //std::unique_ptr<ToasterSubsystem> toasterSubsystem_;
    ToasterSubsystem* toasterSubsystem_;
    ToasterConfig config_;
};


#endif
