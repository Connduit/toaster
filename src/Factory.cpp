#include "Factory.h"
#include "Filters.h"
#include "Sinks.h"

FilterUniquePtr Factory::create(const FilterConfig& config)
{
	switch (config.filterImpl)
	{
	case FilterImplType::IIR:
        switch (config.filterType)
        {
            case FilterType::LOWPASS:
		        return std::make_unique<IIRLowPassFilter>(config.cutoffHz, config.sampleRateHz);
            case FilterType::HIGHPASS:
		        return std::make_unique<IIRHighPassFilter>(config.cutoffHz, config.sampleRateHz);
            case FilterType::BANDPASS:
		        return std::make_unique<IIRBandPassFilter>(config.lowCutoff, config.highCutoff, config.sampleRateHz);
            case FilterType::BANDSTOP:
		        return std::make_unique<IIRBandStopFilter>(config.lowCutoff, config.highCutoff, config.sampleRateHz);
            default:
                return nullptr;
        }
	case FilterImplType::FIR:
        switch (config.filterType)
        {
            case FilterType::LOWPASS:
		        return std::make_unique<FIRLowPassFilter>(config.numTaps, config.cutoffHz, config.sampleRateHz);
            case FilterType::HIGHPASS:
		        return std::make_unique<FIRHighPassFilter>(config.numTaps, config.cutoffHz, config.sampleRateHz);
            case FilterType::BANDPASS:
		        return std::make_unique<FIRBandPassFilter>(config.numTaps, config.lowCutoff, config.highCutoff, config.sampleRateHz);
            case FilterType::BANDSTOP:
		        return std::make_unique<FIRBandStopFilter>(config.numTaps, config.lowCutoff, config.highCutoff, config.sampleRateHz);
            default:
                return nullptr;
        }
	default:
		return nullptr;
	}
}

AudioSinkUniquePtr Factory::create(const SinkConfig &config)
{
    switch (config.audioSinkType)
    {
    case AudioSinkType::WAV:
        return std::make_unique<WavSink>(config.outputWavPath, config.sampleRateHz, config.channels, config.outputGain);
    case AudioSinkType::PCM:
        return std::make_unique<PcmSink>();
    default:
        return nullptr;
    }

}