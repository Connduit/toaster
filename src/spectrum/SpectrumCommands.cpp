#include "spectrum/SpectrumCommands.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <vector>

namespace spectrum_commands {
namespace {

std::string toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

std::vector<std::string> tokenize(const std::string& line) {
    std::istringstream stream(line);
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token) tokens.push_back(toLower(token));
    return tokens;
}

std::string formatHz(double hz) {
    char buffer[48];
    const double magnitude = std::fabs(hz);
    if (magnitude >= 1e6) {
        std::snprintf(buffer, sizeof(buffer), "%.4f MHz", hz / 1e6);
    } else if (magnitude >= 1e3) {
        std::snprintf(buffer, sizeof(buffer), "%.2f kHz", hz / 1e3);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%.0f Hz", hz);
    }
    return buffer;
}

int roundToPowerOfTwo(double value) {
    if (value < 1.0) return 1;
    // Round in the log domain so 1.5x sits halfway between 1x and 2x,
    // which is what "closest zoom level" means to someone typing a span.
    const int exponent = static_cast<int>(std::lround(std::log2(value)));
    return 1 << std::max(0, exponent);
}

// The furthest the *requested* centre is allowed to go: anywhere inside
// the captured band. Whether the view can actually sit there at the
// current span is a separate question, answered by
// SpectrumView::effectiveOffsetHz -- this only rejects asking to look
// somewhere the dongle never received at all.
double clampToCapturedBand(double offsetHz, double rfSampleRateHz) {
    const double limit = rfSampleRateHz / 2.0;
    return std::min(limit, std::max(-limit, offsetHz));
}

std::string describe(const SpectrumView& view, double rfSampleRateHz) {
    const double span = view.spanHz(rfSampleRateHz);
    const double effective = view.effectiveOffsetHz(rfSampleRateHz);
    char buffer[160];
    std::snprintf(buffer, sizeof(buffer), "span %s, centre offset %s (zoom %dx)%s",
                   formatHz(span).c_str(), formatHz(effective).c_str(), view.decimation,
                   std::fabs(effective - view.offsetHz) > 1.0
                       ? "  [zoom in to reach it]"
                       : "");
    return buffer;
}

}  // namespace

bool parseFrequency(const std::string& text, double& hz, bool& isRelative) {
    if (text.empty()) return false;

    size_t index = 0;
    isRelative = false;
    double sign = 1.0;
    if (text[0] == '+' || text[0] == '-') {
        isRelative = true;
        if (text[0] == '-') sign = -1.0;
        index = 1;
    }

    size_t consumed = 0;
    double value = 0.0;
    try {
        value = std::stod(text.substr(index), &consumed);
    } catch (...) {
        return false;
    }
    if (consumed == 0) return false;

    std::string suffix = toLower(text.substr(index + consumed));
    // A trailing "hz" is noise once the multiplier is known: "200khz"
    // and "200k" mean the same thing.
    if (suffix.size() >= 2 && suffix.compare(suffix.size() - 2, 2, "hz") == 0) {
        suffix.erase(suffix.size() - 2);
    }

    double multiplier = 1.0;
    if (suffix == "k") {
        multiplier = 1e3;
    } else if (suffix == "m") {
        multiplier = 1e6;
    } else if (suffix == "g") {
        multiplier = 1e9;
    } else if (!suffix.empty()) {
        return false;
    }

    hz = sign * value * multiplier;
    return true;
}

ViewCommandResult applyCommand(const std::string& line, const SpectrumView& current,
                                double rfCenterHz, double rfSampleRateHz, int maxDecimation) {
    ViewCommandResult result;
    result.view = current;

    const std::vector<std::string> tokens = tokenize(line);
    if (tokens.empty()) return result;

    const std::string& verb = tokens[0];
    if (verb == "help" || verb == "?") {
        // Answered here rather than by the console listener so the
        // commands are documented by the thing that implements them,
        // and so the text lands on the display instead of being drawn
        // over by the next redraw.
        result.recognized = true;
        result.message = helpText();
        return result;
    }

    const bool isZoom = verb == "zoom";
    const bool isCenter = verb == "center" || verb == "centre";
    if (!isZoom && !isCenter) return result;

    result.recognized = true;
    if (tokens.size() < 2) {
        result.message = "usage: " + helpText();
        return result;
    }
    const std::string& argument = tokens[1];

    if (isZoom) {
        if (argument == "in") {
            result.view.decimation = std::min(maxDecimation, current.decimation * 2);
        } else if (argument == "out") {
            result.view.decimation = std::max(1, current.decimation / 2);
        } else if (argument == "reset" || argument == "full" || argument == "off") {
            result.view.decimation = 1;
        } else {
            double requestedSpan = 0.0;
            bool relative = false;
            if (!parseFrequency(argument, requestedSpan, relative) || relative ||
                requestedSpan <= 0.0) {
                result.message = "zoom: expected in, out, reset, or a span like 200k";
                return result;
            }
            result.view.decimation =
                std::min(maxDecimation, roundToPowerOfTwo(rfSampleRateHz / requestedSpan));
        }

        if (result.view.decimation == current.decimation) {
            // Worth saying so explicitly -- silently doing nothing at
            // the end of the zoom range looks like the command was lost.
            result.message = (result.view.decimation == 1 ? "already at full span"
                                                           : "already at maximum zoom") +
                              std::string(" (") + describe(result.view, rfSampleRateHz) + ")";
        }

        // The offset is left exactly as requested. Zooming out may make
        // it temporarily unreachable, but zooming back in should
        // restore it rather than having silently discarded it.
        result.applied = true;
        if (result.message.empty()) result.message = describe(result.view, rfSampleRateHz);
        return result;
    }

    // center
    if (argument == "reset" || argument == "zero") {
        result.view.offsetHz = 0.0;
        result.applied = true;
        result.message = describe(result.view, rfSampleRateHz);
        return result;
    }

    double value = 0.0;
    bool relative = false;
    if (!parseFrequency(argument, value, relative)) {
        result.message = "center: expected a frequency like 100.3M, a shift like +200k, or reset";
        return result;
    }

    // A signed argument shifts the view by that much; an unsigned one
    // names an absolute frequency, which becomes a view offset once the
    // tuned frequency is subtracted. Writing "+200k" and "100.3M" are
    // therefore both natural and mean different things, which is why
    // the sign is tracked through parseFrequency rather than discarded.
    const double requestedOffset = relative ? current.offsetHz + value : value - rfCenterHz;
    const double clamped = clampToCapturedBand(requestedOffset, rfSampleRateHz);

    result.view.offsetHz = clamped;
    result.applied = true;
    if (std::fabs(clamped - requestedOffset) > 1.0) {
        result.message = "outside the captured band; " + describe(result.view, rfSampleRateHz);
    } else {
        result.message = describe(result.view, rfSampleRateHz);
    }
    return result;
}

std::string helpText() {
    // Kept short enough to survive an 80-column header.
    return "zoom in|out|reset|<span>   center <freq>|<+/-shift>|reset   eg: zoom 200k";
}

}  // namespace spectrum_commands
