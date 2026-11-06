#pragma once

#include "spectrum/SpectrumFrame.h"

// Abstract interface for whatever displays finished spectrum frames --
// the direct analogue of AudioSink for the visual side of the pipeline.
//
// SpectrumStage depends only on this interface: it computes frames and
// has no idea whether they end up as coloured bars in a terminal, a
// scrolling waterfall, PNG files on disk, or packets on a socket
// feeding a GUI written in something other than C++. Toaster is the one
// place that picks the concrete renderer (see makeSpectrumSink in
// Toaster.cpp), exactly as it already picks PcmWriter vs WavFileWriter
// for audio.
class SpectrumSink {
public:
    virtual ~SpectrumSink() = default;

    // Draw one frame. Called from the RF read thread, at roughly the
    // configured refresh rate rather than once per FFT window's worth
    // of samples -- see SpectrumStage.h. `frame` is only valid for the
    // duration of the call.
    virtual void renderFrame(const SpectrumFrame& frame) = 0;

    // Finish up: restore any terminal state the renderer changed (the
    // cursor, colours), flush buffered output. Safe to call more than
    // once, and safe to call having never rendered a frame.
    virtual void flush() = 0;
};
