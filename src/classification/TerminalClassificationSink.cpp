#include "classification/TerminalClassificationSink.h"

#include <ctime>

TerminalClassificationSink::TerminalClassificationSink(const std::string& path) {
    if (path == "-") {
        file_ = stderr;
        ownsFile_ = false;
        return;
    }
    file_ = std::fopen(path.c_str(), "w");
    if (!file_) {
        std::fprintf(stderr, "Warning: couldn't open classification log '%s'; writing to stderr instead.\n",
                     path.c_str());
        file_ = stderr;
        ownsFile_ = false;
        return;
    }
    ownsFile_ = true;
}

TerminalClassificationSink::~TerminalClassificationSink() {
    if (ownsFile_ && file_) std::fclose(file_);
}

void TerminalClassificationSink::report(const SignalDetectionMessage& detection) {
    std::time_t now = std::time(nullptr);
    char timestamp[16];
    std::strftime(timestamp, sizeof(timestamp), "%H:%M:%S", std::localtime(&now));

    std::fprintf(file_, "[%s] %.4f MHz  BW %.1f kHz  %.1f dBFS  -> %s (confidence %.2f)\n", timestamp,
                 detection.centerFreqHz / 1e6, detection.bandwidthHz / 1e3, detection.peakDb,
                 detection.label, detection.confidence);
    std::fflush(file_);
}

void TerminalClassificationSink::flush() {
    if (file_) std::fflush(file_);
}
