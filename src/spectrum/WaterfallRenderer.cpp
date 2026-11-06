#include "spectrum/WaterfallRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "spectrum/TerminalCanvas.h"

namespace {

float normalizeDb(float db, const SpectrumFrame& frame) {
    const float range = frame.maxDb - frame.minDb;
    if (!(range > 0.0f)) return 0.0f;
    return std::min(1.0f, std::max(0.0f, (db - frame.minDb) / range));
}

// Stand-in for the colour ramp when escape sequences aren't available
// (output redirected to a file or a pipe). A waterfall with no colour
// would otherwise be a rectangle of blank spaces.
char densityChar(float normalized) {
    static const char kRamp[] = " .:-=+*#%@";
    constexpr int kRampSize = static_cast<int>(sizeof(kRamp)) - 1;  // less the NUL
    if (!(normalized > 0.0f)) return kRamp[0];
    if (normalized >= 1.0f) return kRamp[kRampSize - 1];
    return kRamp[static_cast<int>(normalized * kRampSize)];
}

}  // namespace

std::string WaterfallRenderer::headerText(const SpectrumFrame& frame) const {
    // ASCII only: the base truncates the header by byte count to fit the
    // window, which would cut a multi-byte glyph or an escape sequence
    // in half. The actual colour key is drawn in the body instead,
    // where it isn't subject to truncation.
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "  scale %.0f..%.0f dBFS",
                   static_cast<double>(frame.minDb), static_cast<double>(frame.maxDb));
    return TerminalSpectrumRenderer::headerText(frame) + buffer;
}

void WaterfallRenderer::onLayoutChanged(const Layout& layout) {
    // Rows are cached as rendered strings of a fixed width, so a width
    // change invalidates every one of them. Re-deriving old rows at the
    // new width isn't possible anyway: the dB values they came from
    // were reduced to columns and the originals are long gone.
    history_.clear();
    historyColumns_ = layout.plotColumns;

    // One body row is spent on the colour key at the bottom.
    maxHistoryRows_ = static_cast<size_t>(std::max(1, layout.plotRows - 1));
}

std::string WaterfallRenderer::renderRow(const SpectrumFrame& frame, int columns) {
    reduceToColumns(frame, columns, columnDb_);

    std::string row;
    row.reserve(static_cast<size_t>(columns) * (useAnsi() ? 12 : 1));

    int previousColor = -1;
    for (int column = 0; column < columns; ++column) {
        const float normalized = normalizeDb(columnDb_[static_cast<size_t>(column)], frame);
        if (!useAnsi()) {
            row += densityChar(normalized);
            continue;
        }

        const int color = terminal::heatColor(normalized);
        // Adjacent columns are very often the same colour -- most of a
        // spectrum is noise floor -- so only emit an escape when the
        // colour actually changes. On a quiet band this cuts the row
        // from a few hundred bytes to a few dozen.
        if (color != previousColor) {
            row += terminal::background(color);
            previousColor = color;
        }
        row += ' ';
    }
    if (useAnsi()) row += terminal::resetStyle();
    return row;
}

void WaterfallRenderer::appendBody(std::string& out, const SpectrumFrame& frame,
                                    const Layout& layout) {
    const int columns = layout.plotColumns;

    // Newest frame on top, everything else pushed down one row.
    history_.push_front(renderRow(frame, columns));
    while (history_.size() > maxHistoryRows_) history_.pop_back();

    for (const std::string& row : history_) {
        out += row;
        endLine(out);
    }
    // Until enough frames have arrived to fill the window, pad so the
    // body still occupies exactly plotRows lines and the frequency axis
    // stays put instead of creeping down the screen.
    for (size_t padding = history_.size(); padding < maxHistoryRows_; ++padding) {
        endLine(out);
    }

    // Colour key: the gradient itself with dB values written onto it,
    // which is the only amplitude scale this view has.
    std::string labels(static_cast<size_t>(columns), ' ');
    constexpr int kKeyTicks = 5;
    int nextFreeColumn = 0;
    for (int tick = 0; tick < kKeyTicks; ++tick) {
        const double fraction = static_cast<double>(tick) / (kKeyTicks - 1);
        const double db = frame.minDb + fraction * (frame.maxDb - frame.minDb);

        char text[16];
        std::snprintf(text, sizeof(text), "%.0f", db);
        const int labelWidth = static_cast<int>(std::char_traits<char>::length(text));

        // Same edge clamping as the frequency axis: the -90 and 0
        // labels sit on the first and last columns, so centring them
        // without clamping would push half of each off the display.
        int start = static_cast<int>(std::lround(fraction * (columns - 1))) - labelWidth / 2;
        if (start + labelWidth > columns) start = columns - labelWidth;
        if (start < 0) start = 0;
        if (start < nextFreeColumn) continue;

        labels.replace(static_cast<size_t>(start), static_cast<size_t>(labelWidth), text);
        nextFreeColumn = start + labelWidth + 1;
    }

    int previousColor = -1;
    for (int column = 0; column < columns; ++column) {
        const float fraction = static_cast<float>(column) / static_cast<float>(std::max(1, columns - 1));
        if (!useAnsi()) {
            out += labels[static_cast<size_t>(column)];
            continue;
        }
        const int color = terminal::heatColor(fraction);
        if (color != previousColor) {
            out += terminal::background(color);
            // Dark text on the hot end, light text on the cold end --
            // white numerals on yellow are essentially unreadable.
            out += terminal::foreground(fraction > 0.6f ? 16 : 231);
            previousColor = color;
        }
        out += labels[static_cast<size_t>(column)];
    }
    if (useAnsi()) out += terminal::resetStyle();
    endLine(out);
}
