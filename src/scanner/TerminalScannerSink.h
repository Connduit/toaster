#pragma once

#include <cstdio>
#include <string>

#include "scanner/ScannerSink.h"

// Writes one timestamped line per lock/resume event (and the occasional
// heartbeat) -- an append-only log, not a live-redrawing display.
//
// Defaults to a file rather than stderr for the same reason as
// TerminalClassificationSink (see that class's header comment): stderr
// is where the spectrum-visual stage's live-redrawing renderer lives,
// and any other writer appending plain lines there corrupts its cursor
// math. Passing path == "-" opts into stderr anyway.
class TerminalScannerSink : public ScannerSink {
public:
    explicit TerminalScannerSink(const std::string& path);
    ~TerminalScannerSink() override;

    TerminalScannerSink(const TerminalScannerSink&) = delete;
    TerminalScannerSink& operator=(const TerminalScannerSink&) = delete;

    void report(const ScanEventMessage& event) override;
    void reportStatus(const std::string& message) override;
    void flush() override;

private:
    void printTimestamped(const std::string& line);

    FILE* file_ = nullptr;
    bool ownsFile_ = false;
};
