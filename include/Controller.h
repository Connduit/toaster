//#pragma once
//
////#include "messaging/Dispatcher.h"
////#include "messaging/Messages.h"
////#include "rf/Receiver.h"
//
//// Handles control-plane commands by acting on the actual subsystems.
//// Subscribes to CommandMessages on the Dispatcher rather than being
//// handed a raw callback directly, so any command source (the console
//// listener today, perhaps a remote-control listener tomorrow) works
//// without Controller or its command sources needing to know about each
//// other -- a command source only needs to know how to dispatch a
//// CommandMessage, not who (if anyone) is listening for it.
////
//// This is where command handling lives instead of growing inside
//// Toaster: adding a new command means adding a branch in
//// handleCommand(), not a new method on Toaster that every command
//// source would need to be wired to individually.
//class Controller {
//public:
//    Controller(Dispatcher& dispatcher, Receiver& receiver);
//
//private:
//    void handleCommand(const CommandMessage& command);
//
//    Receiver& receiver_;
//};