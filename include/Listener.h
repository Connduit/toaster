#pragma once

// Base interface for something that runs on a background thread and
// invokes a callback when it observes some external event — a typed
// console command, a filesystem change, a network message, and so on.
// ConsoleStopListener is the FIRst concrete example; a future listener
// (say, one that watches a socket for remote control commands) would
// implement this same interface.
class Listener {
public:
    virtual ~Listener() = default;

    // Starts the background thread that watches for whatever this
    // listener is listening for. Safe to call once.
    virtual void start() = 0;
};
