#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

// One complete spectrum measurement, ready to be drawn.
//
// Like the types in messaging/Messages.h, this is a non-owning view:
// `magnitudesDb` points into a buffer SpectrumStage reuses for the next
// frame, so a renderer that wants to keep a frame around (the waterfall
// does, for its history) must copy what it needs during the call.
//
// The dB scale limits travel with the frame rather than being
// configured separately on each renderer. That's deliberate: they
// describe the measurement ("this data spans -90 to 0 dBFS"), so every
// renderer showing the same frame necessarily agrees on the scale, and
// a renderer needs no configuration of its own to draw a correct axis.
struct SpectrumFrame {
    // binCount entries, ordered lowest frequency first -- i.e. already
    // FFT-shifted, so index 0 is centerFreqHz - spanHz/2 and the last
    // index is just under centerFreqHz + spanHz/2. Values are dBFS: 0
    // means a full-scale complex tone landed entirely in that bin.
    const float* magnitudesDb;
    size_t binCount;

    // Tuned frequency, in Hz -- the middle of the displayed span.
    double centerFreqHz;

    // Total width covered, in Hz. Equal to the RF sample rate: a
    // complex (IQ) stream sampled at fs carries a full fs of bandwidth,
    // not fs/2, because the negative frequencies are distinguishable
    // rather than mirrored as they'd be for a real-valued signal.
    double spanHz;

    // Bottom and top of the dB scale the renderer should map onto its
    // available height/colours. Anything outside is clamped.
    float minDb;
    float maxDb;

    // How far the view is zoomed in: 1 is the full captured band, 8 is
    // an eighth of it at eight times the resolution. Carried so the
    // display can say so; the renderer does nothing else with it.
    int zoom = 1;

    // Optional feedback to show alongside the readout -- confirmation
    // of a command, or why one couldn't be honoured. Null when there's
    // nothing to say. Only valid for the duration of renderFrame().
    //
    // This goes through the frame rather than being printed directly
    // because the display owns the terminal: a stray line written to
    // stderr would land in the middle of the plot and be overwritten by
    // the next redraw anyway.
    const char* status = nullptr;
};

// Frequency, in Hz, at the centre of bin `index` of a frame.
inline double binFrequencyHz(const SpectrumFrame& frame, size_t index) {
    if (frame.binCount == 0) return frame.centerFreqHz;
    const double offsetBins = static_cast<double>(index) - static_cast<double>(frame.binCount) / 2.0;
    return frame.centerFreqHz + offsetBins * frame.spanHz / static_cast<double>(frame.binCount);
}

// Index of the strongest bin in a frame. Zero for an empty frame.
inline size_t peakBin(const SpectrumFrame& frame) {
    size_t best = 0;
    for (size_t bin = 1; bin < frame.binCount; ++bin) {
        if (frame.magnitudesDb[bin] > frame.magnitudesDb[best]) best = bin;
    }
    return best;
}

// Collapses a frame's bins down to `columns` values, writing them into
// `out` (resized as needed, so a caller reusing one buffer across
// frames allocates nothing in the steady state).
//
// Takes the *maximum* dB across each column's range rather than the
// mean. With, say, 1024 bins shown across 70 terminal columns, a narrow
// carrier occupies a single bin; averaging it together with its fifteen
// empty neighbours would drag it most of the way back down to the noise
// floor, hiding precisely the signals a spectrum display exists to
// reveal. Peak-hold-per-column is what every real analyser does when
// it has fewer pixels than bins, for exactly this reason.
inline void reduceToColumns(const SpectrumFrame& frame, int columns, std::vector<float>& out) {
    if (columns <= 0) {
        out.clear();
        return;
    }
    out.assign(static_cast<size_t>(columns), frame.minDb);
    if (frame.binCount == 0) return;

    const size_t binCount = frame.binCount;
    for (int column = 0; column < columns; ++column) {
        size_t begin = static_cast<size_t>(column) * binCount / static_cast<size_t>(columns);
        size_t end = static_cast<size_t>(column + 1) * binCount / static_cast<size_t>(columns);

        // With more columns than bins these ranges can come out empty;
        // widening to one bin makes the display stretch rather than
        // showing gaps.
        if (begin >= binCount) begin = binCount - 1;
        if (end <= begin) end = begin + 1;
        if (end > binCount) end = binCount;

        float peak = frame.magnitudesDb[begin];
        for (size_t bin = begin + 1; bin < end; ++bin) {
            peak = std::max(peak, frame.magnitudesDb[bin]);
        }
        out[static_cast<size_t>(column)] = peak;
    }
}
