#pragma once

#include <cstdio>
#include <string>

#include "classification/ClassificationSink.h"

// Writes one timestamped, human-readable line per detection -- an
// append-only log, not a live-redrawing display.
//
// Defaults to a file rather than stderr for a concrete reason: stderr
// is also where the spectrum-visual stage's renderer lives (see
// spectrum/TerminalSpectrumRenderer.h), and that renderer redraws in
// place using ANSI cursor movement on the assumption that it owns
// everything below where it last drew. Any other writer interleaving
// plain lines into the same stream -- this sink included -- corrupts
// that redraw the next time it runs, regardless of which thread wrote
// what, purely because the cursor math no longer matches what's
// actually on screen. Passing path == "-" opts into stderr anyway (for
// a run that doesn't also have spectrum-visual selected, watching
// detections live rather than tailing a file is often nicer); this
// class does nothing to detect or prevent that conflict, since knowing
// which stages are selected isn't something a sink has (or should need)
// access to -- see Toaster.cpp's makeClassificationSink for where that
// judgment gets made instead.
class TerminalClassificationSink : public ClassificationSink {
public:
    // path == "-" writes to stderr. Otherwise opens (truncating) the
    // given path; failure to open falls back to stderr with a warning,
    // rather than silently discarding every detection.
    explicit TerminalClassificationSink(const std::string& path);
    ~TerminalClassificationSink() override;

    TerminalClassificationSink(const TerminalClassificationSink&) = delete;
    TerminalClassificationSink& operator=(const TerminalClassificationSink&) = delete;

    void report(const SignalDetectionMessage& detection) override;
    void flush() override;

private:
    FILE* file_ = nullptr;
    bool ownsFile_ = false;
};
