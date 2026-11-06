#pragma once

// A single, self-contained consumer of dispatched messages (see
// messaging/Dispatcher.h). Concrete stages subscribe themselves to
// whatever message type(s) they need in their own constructor -- this
// interface only covers the lifecycle every stage still needs
// regardless of what it consumes: flushing buffered output and
// resetting internal state.
//
// Earlier, this interface also had a uniform processBuffer(buf, len),
// and a Pipeline class called it on every stage for every raw IQ
// buffer. That stopped making sense once stages want different kinds
// of data with different shapes (a future classifier might subscribe
// to AudioMessage instead of IQMessage) -- routing by message type
// through Dispatcher replaced it. Toaster still owns each stage (for
// this lifecycle management), it just doesn't drive its per-sample
// processing directly anymore.
class PipelineStage {
public:
    virtual ~PipelineStage() = default;

    // Flush anything this stage has buffered (e.g. audio not yet written
    // out, or a partially-filled FFT window).
    virtual void flush() = 0;

    // Clear internal state back to its initial condition.
    virtual void reset() = 0;
};
