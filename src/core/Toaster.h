#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "console/Console.h"
#include "console/ConsoleCommandListener.h"
#include "core/Config.h"
#include "core/Controller.h"
#include "core/PipelineStage.h"
#include "core/StageRegistry.h"
#include "messaging/Dispatcher.h"
#include "rf/Receiver.h"

// Toaster is the top of the object graph. It owns:
//
//   - a Dispatcher: routes typed messages (raw IQ, audio, commands, ...)
//     from producers to whichever handlers registered for that type
//   - a Receiver: converts raw RF bytes to IQMessages and dispatches them
//   - a Controller: handles CommandMessages (e.g. "stop") by acting on
//     the real subsystems
//   - however many PipelineStages are running (audio output today; a
//     spectrum analyzer or signal classifier potentially tomorrow) --
//     Toaster owns them for lifecycle purposes (flush/reset), but each
//     stage wires its own Dispatcher subscriptions in its own
//     constructor, so Toaster never needs a uniform "processBuffer"
//     call convention across stages that consume different kinds of data
//   - a Console and ConsoleCommandListener: the one built-in command source
//     today, dispatching a CommandMessage on the same Dispatcher rather
//     than calling Toaster directly, so any future command source (a
//     remote-control listener, say) works the same way with no changes
//     needed to Controller or Toaster
//
// This replaces an earlier design where a Pipeline fanned every raw IQ
// buffer out to every stage indiscriminately -- fine when there was
// only ever one (cheap) stage, but wrong once stages want different
// kinds of data (a classifier might want post-demod audio, not raw IQ)
// or have very different costs (a heavy classifier blocking the RF
// thread would stall audio too). Message-type-based routing through
// Dispatcher means a stage only ever receives what it explicitly
// subscribed to.
//
// Nothing else should construct a Receiver, a Controller, or a
// PipelineStage directly -- Toaster is the one place responsible for
// assembling and connecting them.
//
// Which stages actually get built is driven by config.requestedStageKeys
// (see Config.h) against the list returned by allStageDescriptors(): if
// the config didn't pin down a set of keys, the constructor prompts for
// them on the console (see core/StageSelection.h) before building
// anything. This is what lets, say, a future spectrum-analyzer stage and
// audio output be registered as independently selectable rather than
// both always running together.
class Toaster {
public:
    explicit Toaster(const ToasterConfig& config);

    // Opens the receiver and streams data through the Dispatcher to
    // every registered handler until a "stop" command is dispatched
    // (either by ConsoleCommandListener seeing "stop" on stdin, or by
    // stop() below). Blocks the calling thread. Returns false if the
    // receiver could not be opened.
    bool run();

    // Dispatches a "stop" CommandMessage; Controller handles it. Safe
    // to call from any thread.
    void stop();

    // Returns the full set of stages Toaster knows how to build, in a
    // fixed order (the order they're offered in the selection prompt).
    // Registering a new stage means adding one entry here -- nothing
    // else in Toaster changes. Static (rather than a free function)
    // purely so it can live next to the rest of Toaster's assembly logic
    // in Toaster.cpp without polluting an unrelated header.
    static std::vector<StageDescriptor> allStageDescriptors();

private:
    ToasterConfig config_;

    // Which stages resolveStages() settled on, kept so run() can report
    // only on what's actually running rather than assuming.
    std::vector<std::string> selectedStageKeys_;

    Dispatcher dispatcher_;
    Receiver receiver_;
    Controller controller_;
    std::vector<std::unique_ptr<PipelineStage>> stages_;
    Console console_;
    ConsoleCommandListener commandListener_;
};
