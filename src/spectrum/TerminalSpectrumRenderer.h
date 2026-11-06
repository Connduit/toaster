#pragma once

#include <cstdio>
#include <string>

#include "spectrum/SpectrumFrame.h"
#include "spectrum/SpectrumSink.h"

// Everything two different terminal spectrum displays have in common,
// implemented once: measuring the window, deciding whether escape
// sequences are even appropriate, clearing and re-homing the cursor,
// drawing the header readout and the frequency axis, assembling the
// frame into a single buffered write, and restoring the terminal on the
// way out. A subclass supplies only the plot area itself.
//
// This is the same factoring the filter hierarchy already uses (see
// README.md): FirFilter implements the convolution once and its
// subclasses only design coefficients; here the base implements the
// terminal handling once and its subclasses only draw a plot body. The
// alternative -- two independent SpectrumSinks -- meant two copies of
// the axis arithmetic, the resize handling, and the cursor
// save/restore, which is exactly the sort of duplication that drifts
// apart the first time one of them gets a fix.
//
// Writes to a FILE* (stderr by default, never stdout) for the same
// reason Console does: stdout in this program is reserved for raw PCM
// audio, so `-s audio,spectrum-visual | aplay` puts the display on the
// terminal and the audio down the pipe, with neither corrupting the
// other.
class TerminalSpectrumRenderer : public SpectrumSink {
public:
    explicit TerminalSpectrumRenderer(FILE* stream = stderr);
    ~TerminalSpectrumRenderer() override;

    // final, not virtual: the sequence (measure, home, header, body,
    // axis, single write) is the shared part. Subclasses vary the body,
    // not the sequence.
    void renderFrame(const SpectrumFrame& frame) final;

    void flush() override;

protected:
    // Where the base decided everything goes for the current frame.
    // Recomputed per frame, so resizing the terminal is picked up
    // without a SIGWINCH handler -- and therefore without any of the
    // async-signal-safety problems that keep this program off SIGINT
    // as well (see rf/Receiver.h).
    struct Layout {
        int width;         // full terminal width, in columns
        int height;        // full terminal height, in rows
        int gutterWidth;   // columns reserved left of the plot
        int plotRows;      // rows available for the plot body
        int plotColumns;   // columns available for the plot body
    };

    // Draw the plot body: exactly layout.plotRows lines, each finished
    // with endLine(). Called once per frame, between the header and the
    // frequency axis.
    virtual void appendBody(std::string& out, const SpectrumFrame& frame, const Layout& layout) = 0;

    // Text for the header line, truncated by the base to fit the
    // window. The default gives the standard readout (centre, span, bin
    // count, resolution bandwidth, peak); override to add to it.
    virtual std::string headerText(const SpectrumFrame& frame) const;

    // Columns to reserve at the left of the plot, e.g. for a dB scale.
    // Zero (the default) means the plot uses the full width.
    virtual int gutterWidth() const { return 0; }

    // Called whenever the plot's dimensions change (including the first
    // frame), before appendBody. Subclasses caching anything sized to
    // the plot -- the waterfall's history, for one -- invalidate it here.
    virtual void onLayoutChanged(const Layout& layout) { (void)layout; }

    bool useAnsi() const { return useAnsi_; }

    // Terminate a line of the frame: erase whatever the previous frame
    // left to the right of it, then wrap. Without the erase, a frame
    // whose lines are shorter than the last one's leaves the tails of
    // the old frame on screen, since redrawing in place overwrites
    // rather than clears.
    void endLine(std::string& out) const;

    // Shared readouts, so subclasses can build on the default header
    // instead of reimplementing the formatting.
    static std::string formatMhz(double hz);
    static std::string formatHz(double hz);
    // Axis label precision scales with the span; see the .cpp.
    static std::string formatAxisMhz(double hz, double spanHz);

private:
    void appendHeader(std::string& out, const SpectrumFrame& frame, const Layout& layout);
    void appendFrequencyAxis(std::string& out, const SpectrumFrame& frame, const Layout& layout);

    FILE* stream_;
    bool useAnsi_;
    bool screenPrepared_ = false;
    Layout lastLayout_{0, 0, 0, 0, 0};

    // Reused across frames so a steady-state redraw allocates nothing.
    std::string scratch_;
};
