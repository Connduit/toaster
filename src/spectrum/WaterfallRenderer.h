#pragma once

#include <cstdio>
#include <deque>
#include <string>
#include <vector>

#include "spectrum/SpectrumFrame.h"
#include "spectrum/TerminalSpectrumRenderer.h"

// A scrolling waterfall: frequency across, time down, level as colour.
// The newest frame is the top row and older frames march downward until
// they fall off the bottom.
//
// This is the complement to BarSpectrumRenderer rather than a
// decoration of it. A bar trace shows amplitude precisely but only for
// the current instant, so anything intermittent -- a burst transmission,
// a station drifting, interference that comes and goes -- is invisible
// unless you happen to be looking at the exact frame it appears in. A
// waterfall trades amplitude precision (a colour resolves maybe sixteen
// levels; a bar with partial blocks resolves hundreds) for keeping
// every frame on screen, which makes those same signals obvious.
//
// Having two genuinely different views is also what justifies
// SpectrumSink being an abstract interface at all: SpectrumStage
// computes identical frames either way and has no idea which of these
// is attached.
class WaterfallRenderer : public TerminalSpectrumRenderer {
public:
    explicit WaterfallRenderer(FILE* stream = stderr) : TerminalSpectrumRenderer(stream) {}

protected:
    void appendBody(std::string& out, const SpectrumFrame& frame, const Layout& layout) override;

    // Adds the colour legend, since colour is this view's only
    // amplitude scale -- there's no dB gutter to read it off.
    std::string headerText(const SpectrumFrame& frame) const override;

    // History rows are pre-rendered at a fixed width, so a resize
    // invalidates all of them.
    void onLayoutChanged(const Layout& layout) override;

private:
    std::string renderRow(const SpectrumFrame& frame, int columns);

    // Each entry is one fully-rendered row of escape sequences and
    // spaces. Rendering is done once, when a frame arrives, rather than
    // every time that row is redrawn: a row stays on screen for as many
    // frames as there are rows, so re-rendering it each frame would
    // multiply the work by the height of the display for no benefit.
    std::deque<std::string> history_;

    std::vector<float> columnDb_;
    int historyColumns_ = 0;
    size_t maxHistoryRows_ = 0;
};
