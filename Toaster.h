#ifndef TOASTER_H
#define TOASTER_H

#include "ToasterSubsystem.h"

class Toaster
{
public:
    Toaster();
    ~Toaster();
    void createToasterSubsystem();
    void startToasterSubsystem();
    void stopToasterSubsystem();
private:
    static void signalHandler(int sig); // ✅ Static signal handler
    static Toaster *instance_;          // ✅ Static pointer to self
    ToasterSubsystem *toasterSubsystem_;
    // Config config_; // TODO:
};


#endif