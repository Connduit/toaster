#include "Demodulator.h"
#include <cmath>

Demodulator::Demodulator()
    : previousIQ_(0.0f, 0.0f),
      hasPreviousIQ_(false)
{
}

float Demodulator::processSample(
    const std::complex<float>& current)
{
    if (!hasPreviousIQ_)
    {
        previousIQ_ = current;
        hasPreviousIQ_ = true;

        return 0.0f;
    }

    const float audio =
        std::arg(
            current *
            std::conj(previousIQ_)
        );

    previousIQ_ = current;

    return audio;
}

AudioData Demodulator::process(const IQData& iqData)
{
    AudioData audio;

    if (iqData.empty())
        return audio;

    audio.reserve(iqData.size());

    for (const auto& current : iqData)
    {
        // The first sample cannot be demodulated because
        // there is no previous sample yet.
        if (!hasPreviousIQ_)
        {
            previousIQ_ = current;
            hasPreviousIQ_ = true;
            continue;
        }

        const float sample =
            std::arg(
                current *
                std::conj(previousIQ_)
            );

        audio.push_back(sample);

        // IMPORTANT:
        // Preserve this across process() calls.
        previousIQ_ = current;
    }

    return audio;
}