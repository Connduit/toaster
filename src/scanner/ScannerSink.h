#pragma once

#include <string>

#include "messaging/Messages.h"

// Abstract interface for whatever records ScannerStage's lock/resume
// events -- the direct analogue of ClassificationSink and AudioSink for
// this stage's output.
//
// Only lock/resume transitions are reported here, not every step of the
// sweep: a sweep with a short settle/measure time can retune many times
// a second, and a line for every one of them would be noise rather than
// a useful record. ScanEventMessage itself only exists for those
// transitions (see messaging/Messages.h) -- this sink just displays what
// it's given.
class ScannerSink {
public:
    virtual ~ScannerSink() = default;

    virtual void report(const ScanEventMessage& event) = 0;

    // A free-form status line -- used for the low-rate "still scanning,
    // now at X" heartbeat (see ScannerStage.h), which isn't a lock/resume
    // event and so doesn't fit ScanEventMessage's shape.
    virtual void reportStatus(const std::string& message) = 0;

    // Flush any buffered output. Safe to call more than once.
    virtual void flush() = 0;
};
