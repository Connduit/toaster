#include "spectrum/TerminalSpectrumRenderer.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "spectrum/TerminalCanvas.h"

namespace {

// Rows spent on things that aren't the plot body: the header, the axis
// rule, the frequency labels, and one blank row left at the bottom so
// the shell prompt doesn't land glued against the display on exit.
constexpr int kNonPlotRows = 4;

// Below these the display stops being meaningful; clamping rather than
// refusing to draw keeps a briefly-tiny window (mid-resize) from
// producing garbage or a crash.
constexpr int kMinWidth = 32;
constexpr int kMinHeight = 10;
constexpr int kMinPlotRows = 4;
constexpr int kMinPlotColumns = 8;

}  // namespace

TerminalSpectrumRenderer::TerminalSpectrumRenderer(FILE* stream)
    : stream_(stream), useAnsi_(terminal::isInteractive(fileno(stream))) {}

TerminalSpectrumRenderer::~TerminalSpectrumRenderer() { flush(); }

std::string TerminalSpectrumRenderer::formatMhz(double hz) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.3f", hz / 1e6);
    return buffer;
}

// Axis labels need enough decimal places to actually distinguish
// neighbouring ticks. At full span three places is right; zoomed to a
// 30 kHz span every tick would otherwise read the same, so the
// precision follows the span rather than being fixed.
std::string TerminalSpectrumRenderer::formatAxisMhz(double hz, double spanHz) {
    int decimals = 3;
    if (spanHz < 2e3) {
        decimals = 6;
    } else if (spanHz < 20e3) {
        decimals = 5;
    } else if (spanHz < 200e3) {
        decimals = 4;
    }
    char buffer[40];
    std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, hz / 1e6);
    return buffer;
}

std::string TerminalSpectrumRenderer::formatHz(double hz) {
    char buffer[32];
    if (hz >= 1e6) {
        std::snprintf(buffer, sizeof(buffer), "%.3f MHz", hz / 1e6);
    } else if (hz >= 1000.0) {
        std::snprintf(buffer, sizeof(buffer), "%.1f kHz", hz / 1000.0);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%.0f Hz", hz);
    }
    return buffer;
}

void TerminalSpectrumRenderer::endLine(std::string& out) const {
    if (useAnsi_) out += terminal::eraseToLineEnd();
    out += "\n";
}

std::string TerminalSpectrumRenderer::headerText(const SpectrumFrame& frame) const {
    // Resolution bandwidth: how far apart two tones must be before this
    // display shows them as two. It's wider than the raw bin spacing
    // because the Hann window spreads each tone over more than one bin
    // -- Hann's equivalent noise bandwidth is 1.5 bins, so quoting the
    // bin spacing alone would overstate what the display can resolve.
    const double resolutionBandwidthHz =
        frame.binCount > 0 ? 1.5 * frame.spanHz / static_cast<double>(frame.binCount) : 0.0;

    const size_t peak = peakBin(frame);
    const double peakDb = frame.binCount > 0 ? frame.magnitudesDb[peak] : 0.0;

    // The span is quoted in whatever unit reads naturally: at full zoom
    // it's megahertz, but by 64x it's a few tens of kilohertz and
    // "0.030 MHz" is a poor way to say 30 kHz.
    char zoomText[32] = "";
    if (frame.zoom > 1) std::snprintf(zoomText, sizeof(zoomText), "  zoom %dx", frame.zoom);

    char buffer[256];
    std::snprintf(buffer, sizeof(buffer),
                   "%s MHz  span %s  %zu bins  RBW %s%s  peak %s MHz @ %.1f dB",
                   formatMhz(frame.centerFreqHz).c_str(), formatHz(frame.spanHz).c_str(),
                   frame.binCount, formatHz(resolutionBandwidthHz).c_str(), zoomText,
                   formatMhz(binFrequencyHz(frame, peak)).c_str(), peakDb);
    return buffer;
}

void TerminalSpectrumRenderer::appendHeader(std::string& out, const SpectrumFrame& frame,
                                             const Layout& layout) {
    // The live display overwrites whatever Toaster and
    // ConsoleCommandListener printed at startup, including the "type
    // 'stop'" hint -- so it gets repeated here, where it survives.
    // Command feedback replaces the hint when there is any: it's
    // transient and the user just asked for it, so it's the more
    // useful thing to be showing in that space.
    //
    // Truncation by byte count is only safe because everything the
    // default header builds is ASCII. A subclass adding multi-byte
    // glyphs should keep them out of headerText() (the waterfall's
    // colour legend is drawn in its body for exactly this reason).
    const std::string trailer =
        frame.status != nullptr ? "  >> " + std::string(frame.status)
                                 : std::string("   ('help' for commands, 'stop' quits)");

    // The trailer gets the space it needs and the readout yields, not
    // the other way round. Truncating the whole line instead would cut
    // the feedback off the right-hand end -- which is exactly the part
    // the user just asked to see, and the part that isn't repeated on
    // the next frame.
    std::string header = headerText(frame);
    const int room = layout.width - static_cast<int>(trailer.size());
    if (room < 0) {
        header = trailer.substr(0, static_cast<size_t>(layout.width));
    } else {
        if (static_cast<int>(header.size()) > room) header.resize(static_cast<size_t>(room));
        header += trailer;
    }
    out += header;
    endLine(out);
}

void TerminalSpectrumRenderer::appendFrequencyAxis(std::string& out, const SpectrumFrame& frame,
                                                    const Layout& layout) {
    const int columns = layout.plotColumns;
    constexpr int kTickCount = 5;

    std::vector<int> tickColumns;
    tickColumns.reserve(kTickCount);
    for (int tick = 0; tick < kTickCount; ++tick) {
        const double fraction = static_cast<double>(tick) / (kTickCount - 1);
        tickColumns.push_back(static_cast<int>(std::lround(fraction * (columns - 1))));
    }

    // The rule, with a tick at each labelled position.
    if (layout.gutterWidth > 0) {
        out.append(static_cast<size_t>(layout.gutterWidth - 1), ' ');
        out += "\u2514";  // corner, joining the gutter's vertical rule
    }
    for (int column = 0; column < columns; ++column) {
        const bool isTick =
            std::find(tickColumns.begin(), tickColumns.end(), column) != tickColumns.end();
        out += isTick ? "\u252c" : "\u2500";
    }
    endLine(out);

    // The labels, centred under their ticks. Built on a plain ASCII
    // canvas so a column index and a byte offset are the same thing --
    // which they would not be if the multi-byte rule characters above
    // shared this buffer.
    std::string labels(static_cast<size_t>(columns), ' ');
    int nextFreeColumn = 0;
    for (int tick = 0; tick < kTickCount; ++tick) {
        const int tickColumn = tickColumns[static_cast<size_t>(tick)];
        const double fraction = static_cast<double>(tickColumn) / std::max(1, columns - 1);
        const double frequencyHz = frame.centerFreqHz + (fraction - 0.5) * frame.spanHz;
        const std::string label = formatAxisMhz(frequencyHz, frame.spanHz);
        const int labelWidth = static_cast<int>(label.size());

        // Centre on the tick, then pull back inside the plot at both
        // ends -- the first and last labels are centred on columns 0
        // and columns-1, so half of each would otherwise fall outside
        // the display and the label would be dropped entirely.
        int start = tickColumn - labelWidth / 2;
        if (start + labelWidth > columns) start = columns - labelWidth;
        if (start < 0) start = 0;
        // Drop a label rather than let it overwrite its neighbour: on a
        // narrow terminal, three readable labels beat five mangled ones.
        if (start < nextFreeColumn) continue;

        labels.replace(static_cast<size_t>(start), label.size(), label);
        nextFreeColumn = start + labelWidth + 1;
    }

    out.append(static_cast<size_t>(layout.gutterWidth), ' ');
    out += labels;
    endLine(out);
}

void TerminalSpectrumRenderer::renderFrame(const SpectrumFrame& frame) {
    const terminal::Size size = terminal::querySize(fileno(stream_));

    Layout layout;
    layout.width = std::max(size.columns, kMinWidth);
    layout.height = std::max(size.rows, kMinHeight);
    layout.gutterWidth = gutterWidth();
    layout.plotRows = std::max(layout.height - kNonPlotRows, kMinPlotRows);
    layout.plotColumns = std::max(layout.width - layout.gutterWidth, kMinPlotColumns);

    // Cleared unconditionally, before anything is appended: making this
    // the responsibility of one branch of the resize check below is how
    // a frame ends up appended to the previous one.
    scratch_.clear();

    if (layout.plotRows != lastLayout_.plotRows || layout.plotColumns != lastLayout_.plotColumns) {
        onLayoutChanged(layout);
        // A resize leaves the old, larger frame's leftovers on screen
        // in regions the new one never writes to; clearing once is
        // simpler and cheaper than tracking which cells went stale.
        if (useAnsi_ && screenPrepared_) scratch_ += terminal::clearScreen();
        lastLayout_ = layout;
    }

    if (useAnsi_) {
        if (!screenPrepared_) {
            scratch_ += terminal::hideCursor();
            scratch_ += terminal::clearScreen();
            screenPrepared_ = true;
        }
        scratch_ += terminal::cursorHome();
    }

    appendHeader(scratch_, frame, layout);
    appendBody(scratch_, frame, layout);
    appendFrequencyAxis(scratch_, frame, layout);

    // Without cursor addressing there's nothing separating consecutive
    // frames, so they'd run together into one wall of text.
    if (!useAnsi_) scratch_ += "\n";

    // One write per frame. Drawing cell-by-cell with many small writes
    // is the usual cause of visible tearing in a terminal UI: the
    // terminal repaints whenever it likes, and a half-delivered frame
    // is a half-drawn frame.
    std::fwrite(scratch_.data(), 1, scratch_.size(), stream_);
    std::fflush(stream_);
}

void TerminalSpectrumRenderer::flush() {
    if (!screenPrepared_) return;
    screenPrepared_ = false;  // idempotent: a second flush() does nothing

    if (useAnsi_) {
        std::fputs(terminal::resetStyle(), stream_);
        std::fputs(terminal::showCursor(), stream_);
        std::fputc('\n', stream_);
    }
    std::fflush(stream_);
}
