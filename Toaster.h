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
    // void initToasterSubsystem(); wrapper function to explicitly call rtlsdr_read_async?
    void startToasterSubsystem();
    void stopToasterSubsystem();
private:
    static void signalHandler(int sig); // Static signal handler
    //static Toaster *instance_;          // Static pointer to self
    ToasterSubsystem *toasterSubsystem_;
    Config config_; // TODO:
};


#endif
