#pragma once

#include <cstdio>
#include <string>
#include <vector>

#include "spectrum/SpectrumFrame.h"
#include "spectrum/TerminalSpectrumRenderer.h"

// The classic spectrum-analyser trace: signal level against frequency,
// as a row of coloured vertical bars redrawn in place. Shows the
// spectrum *right now* in full detail, at the cost of showing nothing
// about the past -- a burst that came and went between two frames
// leaves no trace. (WaterfallRenderer is the complementary view: less
// amplitude detail, but every frame retained for as long as it fits on
// screen.)
//
// Everything about driving a terminal lives in the base class; this
// subclass adds a dB scale down the left-hand side and the bars
// themselves.
class BarSpectrumRenderer : public TerminalSpectrumRenderer {
public:
    explicit BarSpectrumRenderer(FILE* stream = stderr) : TerminalSpectrumRenderer(stream) {}

protected:
    void appendBody(std::string& out, const SpectrumFrame& frame, const Layout& layout) override;

    // Width of the left-hand dB scale, including its separator column.
    int gutterWidth() const override { return 7; }

private:
    // Reused across frames so a steady-state redraw allocates nothing.
    std::vector<float> columnDb_;
};
