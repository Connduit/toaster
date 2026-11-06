#pragma once

// Abstract interface for whatever consumes the pipeline's final,
// post-deemphasis audio samples. AudioPipeline depends only on this
// interface — it has no idea whether samples end up as raw PCM piped to
// something like `aplay`, in a .wav file on disk, or somewhere else
// entirely.
class AudioSink {
public:
    virtual ~AudioSink() = default;

    // Consume one sample, roughly in [-1, 1] before any gain the sink
    // applies internally.
    virtual void pushSample(float sample) = 0;

    // Flush/finalize anything buffered. Safe to call multiple times.
    virtual void flush() = 0;
};
