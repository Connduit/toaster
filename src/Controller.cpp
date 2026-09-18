////#include "core/Controller.h"
////
////#include "messaging/Messages.h"
//
//Controller::Controller(Dispatcher& dispatcher, Receiver& receiver) : receiver_(receiver) {
//    dispatcher.registerTypedHandler<CommandMessage>(
//        [this](const CommandMessage& command) { handleCommand(command); });
//}
//
//void Controller::handleCommand(const CommandMessage& command) {
//    if (command.name == "stop") {
//        receiver_.stop();
//    }
//    // Future commands (retune, switch anti-alias filter implementation,
//    // adjust gain, ...) add a branch here rather than a new method
//    // scattered across Toaster.
//}