#include "core/Controller.h"

#include "messaging/Messages.h"
#include "spectrum/SpectrumCommands.h"

namespace {
constexpr const char* kTunePrefix = "tune ";
}

Controller::Controller(Dispatcher& dispatcher, Receiver& receiver) : receiver_(receiver) {
    dispatcher.registerTypedHandler<CommandMessage>(
        [this](const CommandMessage& command) { handleCommand(command); });
}

void Controller::handleCommand(const CommandMessage& command) {
    if (command.name == "stop") {
        receiver_.stop();
    } else if (command.name.rfind(kTunePrefix, 0) == 0) {
        // Reuses spectrum_commands::parseFrequency (a free function with
        // no dependency on SpectrumStage or a dispatcher -- see
        // spectrum/SpectrumCommands.h) rather than a second frequency
        // parser, so "tune 100.3M" and "tune 100300000" both work today
        // even though ScannerStage -- the only current producer -- only
        // ever emits the plain-Hz form. That leaves room for a future
        // human-typed "tune" console command with no parser changes.
        // Relative deltas ("tune +100k") aren't a meaningful "retune the
        // hardware to here" instruction, so they're rejected rather than
        // guessed at.
        std::string argument = command.name.substr(std::string(kTunePrefix).size());
        double freqHz = 0.0;
        bool isRelative = false;
        if (spectrum_commands::parseFrequency(argument, freqHz, isRelative) && !isRelative) {
            receiver_.retune(freqHz);
        }
    }
    // Future commands (switch anti-alias filter implementation, adjust
    // gain, ...) add a branch here rather than a new method scattered
    // across Toaster.
}
