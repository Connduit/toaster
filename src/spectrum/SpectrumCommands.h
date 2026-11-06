#pragma once

#include <string>

#include "spectrum/SpectrumView.h"

// Parsing for the console commands that drive the spectrum display, kept
// separate from SpectrumStage so it can be tested without a dispatcher,
// a receiver, or a terminal attached.
namespace spectrum_commands {

// Reads a frequency written the way a radio user would write one:
// "100.3M", "200k", "7500", "1.2G", optionally with a trailing "hz".
// A leading + or - marks it as a relative amount. Returns false if the
// text isn't a frequency at all.
bool parseFrequency(const std::string& text, double& hz, bool& isRelative);

// The outcome of interpreting one console line.
struct ViewCommandResult {
    // False when the line wasn't a spectrum command at all, in which
    // case the caller should ignore it -- other subscribers (Controller,
    // handling "stop") may want it.
    bool recognized = false;

    // True when it was a spectrum command and produced a usable new
    // view. False with `message` set means it was recognized but
    // malformed, or asked for something out of range.
    bool applied = false;

    SpectrumView view;

    // Human-readable feedback to show on the display: an error, or a
    // confirmation of what the view became.
    std::string message;
};

// Interprets `line` against the current view. `rfCenterHz` is what the
// dongle is tuned to, needed to turn an absolute frequency like
// "100.3M" into a view offset. `rfSampleRateHz` and `maxDecimation`
// bound how far the view is allowed to move, since the view can only
// ever be a slice of what the dongle actually captured.
ViewCommandResult applyCommand(const std::string& line, const SpectrumView& current,
                                double rfCenterHz, double rfSampleRateHz, int maxDecimation);

// One-line summary of the available commands, for the display's help.
std::string helpText();

}  // namespace spectrum_commands
