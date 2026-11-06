#include "scanner/TerminalScannerSink.h"

#include <ctime>

TerminalScannerSink::TerminalScannerSink(const std::string& path) {
    if (path == "-") {
        file_ = stderr;
        ownsFile_ = false;
        return;
    }
    file_ = std::fopen(path.c_str(), "w");
    if (!file_) {
        std::fprintf(stderr, "Warning: couldn't open scan log '%s'; writing to stderr instead.\n",
                     path.c_str());
        file_ = stderr;
        ownsFile_ = false;
        return;
    }
    ownsFile_ = true;
}

TerminalScannerSink::~TerminalScannerSink() {
    if (ownsFile_ && file_) std::fclose(file_);
}

void TerminalScannerSink::printTimestamped(const std::string& line) {
    std::time_t now = std::time(nullptr);
    char timestamp[16];
    std::strftime(timestamp, sizeof(timestamp), "%H:%M:%S", std::localtime(&now));
    std::fprintf(file_, "[%s] %s\n", timestamp, line.c_str());
    std::fflush(file_);
}

void TerminalScannerSink::report(const ScanEventMessage& event) {
    char buffer[128];
    if (event.locked) {
        std::snprintf(buffer, sizeof(buffer), "LOCKED @ %.4f MHz, %.1f dBFS", event.freqHz / 1e6,
                      event.powerDb);
    } else {
        std::snprintf(buffer, sizeof(buffer), "Resuming scan past %.4f MHz", event.freqHz / 1e6);
    }
    printTimestamped(buffer);
}

void TerminalScannerSink::reportStatus(const std::string& message) { printTimestamped(message); }

void TerminalScannerSink::flush() {
    if (file_) std::fflush(file_);
}
