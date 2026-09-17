#pragma once

#include <algorithm>
#include <cctype>
#include <functional>
#include <string>
#include <thread>
#include <utility>

#include "Console.h"
#include "Listener.h"

// Watches the console on a background thread for the user typing "stop"
// (case-insensitive) followed by Enter, and invokes a callback when it
// sees it.
//
// This exists instead of relying on Ctrl+C (SIGINT): stopping an
// librtlsdr async read loop means calling rtlsdr_cancel_async(), and a
// real signal handler is not a safe place to do that — it can fire in
// the middle of a stdio call or a USB transfer and leave things in a
// bad state. Reading a command on an ordinary background thread calls
// the exact same stop function, just from a normal thread instead of a
// signal context, which is safe.

// TODO: make corresponding .cpp file for this class
class ConsoleStopListener : public Listener 
{
public:
    using StopCallback = std::function<void()>;

    ConsoleStopListener(Console& console, StopCallback onStop)
        : console_(console), onStop_(std::move(onStop)) {}

    // Starts a detached background thread that watches the console. Safe
    // to call once. The thread exits on its own once it sees "stop" (or
    // the input stream closes), and is deliberately detached rather than
    // joined since it may still be blocked waiting for a line when the
    // program is ready to exit — the OS reclaims it at process exit
    // either way.
    void start() override 
    {
        console_.printLine("Type 'stop' and press Enter to stop.");
        std::thread([this]() { run(); }).detach();
    }

private:
    void run() 
    {
        std::string line;
        while (console_.readLine(line)) 
        {
            if (toLower(trim(line)) == "stop") 
            {
                if (onStop_)
                {
                    onStop_();
                }
                return;
            }
        }
        // Input stream closed (EOF) — treat that as a stop request too,
        // so redirecting stdin from /dev/null doesn't hang the program
        // forever with no way to end it.
        if (onStop_) 
        {
            onStop_();
        }
    }

    static std::string trim(const std::string& s) 
    {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    static std::string toLower(std::string s) 
    {
        std::transform(s.begin(), s.end(), s.begin(),
                        [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    Console& console_;
    StopCallback onStop_;
};
