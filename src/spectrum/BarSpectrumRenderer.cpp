#include "spectrum/BarSpectrumRenderer.h"

#include <algorithm>
#include <cmath>

#include "spectrum/TerminalCanvas.h"

namespace {

// Where `db` sits on the frame's scale, as 0..1.
float normalizeDb(float db, const SpectrumFrame& frame) {
    const float range = frame.maxDb - frame.minDb;
    if (!(range > 0.0f)) return 0.0f;
    return std::min(1.0f, std::max(0.0f, (db - frame.minDb) / range));
}

}  // namespace

void BarSpectrumRenderer::appendBody(std::string& out, const SpectrumFrame& frame,
                                      const Layout& layout) {
    reduceToColumns(frame, layout.plotColumns, columnDb_);
    const int columns = static_cast<int>(columnDb_.size());
    const int plotRows = layout.plotRows;

    // Label every few rows rather than all of them: a dB number against
    // every single row is dense enough to read worse than none at all.
    const int labelStride = std::max(1, plotRows / 6);

    // Bar heights are computed once, in eighths of a character cell,
    // rather than re-derived inside the row loop -- the row loop reads
    // every column's height on every row, so deriving it there would
    // repeat the same lround() plotRows times per column.
    std::vector<int> columnEighths(static_cast<size_t>(columns));
    for (int column = 0; column < columns; ++column) {
        const float normalized = normalizeDb(columnDb_[static_cast<size_t>(column)], frame);
        columnEighths[static_cast<size_t>(column)] =
            static_cast<int>(std::lround(normalized * static_cast<float>(plotRows) * 8.0f));
    }

    for (int row = 0; row < plotRows; ++row) {
        const int rowsFromBottom = plotRows - 1 - row;

        // Left gutter: the dB level at this row's lower edge.
        char gutter[16];
        if (rowsFromBottom % labelStride == 0) {
            const float level = frame.minDb + (frame.maxDb - frame.minDb) *
                                                  static_cast<float>(rowsFromBottom) /
                                                  static_cast<float>(plotRows);
            std::snprintf(gutter, sizeof(gutter), "%5.0f ", static_cast<double>(level));
        } else {
            std::snprintf(gutter, sizeof(gutter), "%*s", layout.gutterWidth - 1, "");
        }
        out += gutter;
        out += "\u2502";  // vertical rule separating the scale from the plot

        // One colour per row, not per cell. The colour encodes height on
        // the dB scale, which is by definition constant across a row, so
        // this is a single escape sequence per row instead of one per
        // character cell -- roughly a seventieth of the bytes on a
        // display being redrawn twenty times a second.
        if (useAnsi()) {
            const float rowLevel =
                (static_cast<float>(rowsFromBottom) + 0.5f) / static_cast<float>(plotRows);
            out += terminal::foreground(terminal::heatColor(rowLevel));
        }

        for (int column = 0; column < columns; ++column) {
            // How much of this cell the bar fills: everything below this
            // row is already accounted for, so subtract it off and clamp
            // to a whole cell. eighthBlock() does the clamping.
            const int cellEighths = columnEighths[static_cast<size_t>(column)] - rowsFromBottom * 8;
            out += terminal::eighthBlock(cellEighths);
        }

        if (useAnsi()) out += terminal::resetStyle();
        endLine(out);
    }
}
