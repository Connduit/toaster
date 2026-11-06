#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <thread>

#include "console/Console.h"
#include "console/Listener.h"
#include "messaging/Dispatcher.h"
#include "messaging/Messages.h"

// Watches the console on a background thread and dispatches whatever the
// user types as a CommandMessage.
//
// Replaces the earlier ConsoleStopListener, which recognized exactly one
// word. That was fine while "stop" was the only command, but it meant
// this class had to be taught about every new command as it appeared --
// and it has no business knowing what commands exist. Forwarding lines
// verbatim instead means adding a command is purely a matter of some
// subscriber choosing to handle it: "zoom in" reaches SpectrumStage and
// "stop" reaches Controller without either of them, or this class, ever
// being told about the other.
//
// Dispatching (instead of relying on Ctrl+C/SIGINT) exists because
// stopping an librtlsdr async read loop means calling
// rtlsdr_cancel_async(), and a real signal handler is not a safe place
// to do that -- it can fire in the middle of a stdio call or a USB
// transfer and leave things in a bad state. Reading commands on an
// ordinary background thread avoids all of that.
//
// Note that handlers are invoked on *this* thread, not the RF thread.
// A subscriber that shares state with the sample path is responsible
// for its own synchronization; SpectrumStage's threading note describes
// how it does that.
class ConsoleCommandListener : public Listener {
public:
    ConsoleCommandListener(Console& console, Dispatcher& dispatcher)
        : console_(console), dispatcher_(dispatcher) {}

    // Starts a detached background thread that watches the console. Safe
    // to call once. The thread exits on its own once it sees "stop" (or
    // the input stream closes), and is deliberately detached rather than
    // joined since it may still be blocked waiting for a line when the
    // program is ready to exit -- the OS reclaims it at process exit
    // either way.
    void start() override {
        console_.printLine("Type 'help' for commands, or 'stop' and Enter to stop.");
        std::thread([this]() { run(); }).detach();
    }

private:
    void run() {
        std::string line;
        while (console_.readLine(line)) {
            const std::string command = trim(line);
            if (command.empty()) continue;

            dispatcher_.dispatch(CommandMessage{command});

            // Recognized here as well as by Controller, purely to know
            // when to stop reading. Acting on it is still Controller's
            // job -- this thread only needs to know it should exit.
            if (toLower(command) == "stop") return;
        }
        // Input stream closed (EOF) -- treat that as a stop request too,
        // so redirecting stdin from /dev/null doesn't hang the program
        // forever with no way to end it.
        dispatcher_.dispatch(CommandMessage{"stop"});
    }

    static std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    static std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                        [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    Console& console_;
    Dispatcher& dispatcher_;
};
