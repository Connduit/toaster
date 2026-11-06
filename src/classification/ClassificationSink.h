#pragma once

#include "messaging/Messages.h"

// Abstract interface for whatever records/displays classification
// detections -- the direct analogue of AudioSink and SpectrumSink for
// this stage's output.
//
// ClassificationStage depends only on this: it measures signals and
// matches them against the device catalog, and has no idea whether the
// result ends up as lines in a log file, on a terminal, or somewhere
// else. Toaster is the one place that picks the concrete sink (see
// makeClassificationSink in Toaster.cpp).
//
// Reusing SignalDetectionMessage as the parameter type here (rather
// than inventing a second, near-identical struct) means a sink and a
// Dispatcher subscriber see exactly the same data -- ClassificationStage
// both calls this sink directly *and* dispatches the same struct as a
// SignalDetectionMessage (see ClassificationStage.cpp), so nothing here
// commits a sink to caring about the Dispatcher.
class ClassificationSink {
public:
    virtual ~ClassificationSink() = default;

    // One detection. `detection.label` is safe to keep past the call
    // (see SignalDetectionMessage's comment in messaging/Messages.h).
    virtual void report(const SignalDetectionMessage& detection) = 0;

    // Flush any buffered output. Safe to call more than once.
    virtual void flush() = 0;
};
