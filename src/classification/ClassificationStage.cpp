#include "classification/ClassificationStage.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "classification/DeviceCatalog.h"
#include "dsp/WindowFunction.h"
#include "spectrum/SpectrumCommands.h"

namespace {

// See SpectrumStage.cpp's identical constant for why this exists: a
// silent (all-zero) window produces an exactly-zero bin, and this floor
// keeps its log10 finite rather than producing -infinity.
constexpr float kPowerFloor = 1e-20f;

constexpr const char* kTunePrefix = "tune ";

}  // namespace

ClassificationStage::ClassificationStage(const ClassificationStageConfig& config, Dispatcher& dispatcher,
                                          std::unique_ptr<ClassificationSink> sink)
    : config_(config),
      fft_(static_cast<size_t>(config.fftSize > 0 ? config.fftSize : 1)),
      dispatcher_(dispatcher),
      sink_(std::move(sink)) {
    const size_t size = fft_.size();

    windowCoefficients_ = window::makeHann(size);
    const double windowSum = window::coherentGainSum(windowCoefficients_);
    powerNormalization_ = static_cast<float>(1.0 / (windowSum * windowSum));

    fftBuffer_.resize(size);
    magnitudesDb_.assign(size, -200.0f);
    noiseFloorScratch_.assign(size, -200.0f);

    // Epoch, so the first window is immediately due rather than waiting
    // one refresh interval after startup -- same reasoning as
    // SpectrumStage's identical line.
    lastFrameTime_ = std::chrono::steady_clock::time_point{};

    dispatcher.registerTypedHandler<IQMessage>(
        [this](const IQMessage& message) { onIQMessage(message); });
    dispatcher.registerTypedHandler<CommandMessage>(
        [this](const CommandMessage& command) { onCommand(command); });
}

void ClassificationStage::onCommand(const CommandMessage& command) {
    if (command.name.rfind(kTunePrefix, 0) != 0) return;

    // Reuses spectrum_commands::parseFrequency purely as a stateless
    // string-to-Hz parser (see its header: no dependency on
    // SpectrumStage, a dispatcher, or a receiver) -- not a dependency on
    // spectrum/'s view logic, which is exactly what this class's header
    // comment explains staying independent of.
    std::string argument = command.name.substr(std::string(kTunePrefix).size());
    double freqHz = 0.0;
    bool isRelative = false;
    if (!spectrum_commands::parseFrequency(argument, freqHz, isRelative) || isRelative) return;

    std::lock_guard<std::mutex> lock(tuneMutex_);
    requestedCenterFreqHz_ = freqHz;
    tuneRequestPending_ = true;
}

bool ClassificationStage::frameDue() const {
    const auto elapsed = std::chrono::steady_clock::now() - lastFrameTime_;
    return elapsed >= std::chrono::milliseconds(config_.refreshIntervalMs);
}

void ClassificationStage::applyPendingTune() {
    std::lock_guard<std::mutex> lock(tuneMutex_);
    if (!tuneRequestPending_) return;
    config_.centerFreqHz = requestedCenterFreqHz_;
    tuneRequestPending_ = false;
}

void ClassificationStage::beginWindow() {
    collecting_ = true;
    fill_ = 0;
}

bool ClassificationStage::fillWindow(const std::complex<float>* samples, size_t count) {
    const size_t size = fft_.size();
    const size_t take = std::min(size - fill_, count);
    std::copy(samples, samples + take, fftBuffer_.begin() + static_cast<std::ptrdiff_t>(fill_));
    fill_ += take;
    return fill_ == size;
}

void ClassificationStage::onIQMessage(const IQMessage& message) {
    if (!collecting_) {
        if (!frameDue()) return;
        applyPendingTune();
        beginWindow();
    }

    if (!fillWindow(message.samples, message.count)) return;  // window spans more messages

    processWindow();
    collecting_ = false;
    fill_ = 0;
    lastFrameTime_ = std::chrono::steady_clock::now();
}

float ClassificationStage::estimateNoiseFloorDb() {
    noiseFloorScratch_ = magnitudesDb_;
    size_t percentileIndex = noiseFloorScratch_.size() / 5;  // ~20th percentile
    std::nth_element(noiseFloorScratch_.begin(), noiseFloorScratch_.begin() + percentileIndex,
                      noiseFloorScratch_.end());
    return noiseFloorScratch_[percentileIndex];
}

void ClassificationStage::emitDetection(size_t runStart, size_t runEnd, double binWidthHz) {
    // Power-weighted centroid, in linear power (not dB -- see
    // SpectrumStage.cpp's identical reasoning on why averaging happens
    // in linear power) across just this run's bins.
    double weightedIndexSum = 0.0;
    double weightSum = 0.0;
    float peakDb = magnitudesDb_[runStart];
    for (size_t i = runStart; i < runEnd; ++i) {
        double linearPower = std::pow(10.0, static_cast<double>(magnitudesDb_[i]) / 10.0);
        weightedIndexSum += static_cast<double>(i) * linearPower;
        weightSum += linearPower;
        peakDb = std::max(peakDb, magnitudesDb_[i]);
    }
    double weightedIndex =
        weightSum > 0.0 ? weightedIndexSum / weightSum : static_cast<double>(runStart + runEnd) / 2.0;

    // Same FFT-shift-to-absolute-frequency formula as
    // spectrum/SpectrumFrame.h's binFrequencyHz(), inlined rather than
    // shared: that header lives in spectrum/, and this class deliberately
    // has no dependency there (see this class's header comment).
    const double binCount = static_cast<double>(fft_.size());
    const double offsetBins = weightedIndex - binCount / 2.0;
    const double spanHz = static_cast<double>(config_.rfSampleRateHz);
    const double centerFreqHz = config_.centerFreqHz + offsetBins * spanHz / binCount;
    const double bandwidthHz = static_cast<double>(runEnd - runStart) * binWidthHz;

    classification::MatchResult match = classification::matchDevice(centerFreqHz, bandwidthHz);

    SignalDetectionMessage detection;
    detection.centerFreqHz = centerFreqHz;
    detection.bandwidthHz = bandwidthHz;
    detection.peakDb = peakDb;
    detection.label = match.label;
    detection.confidence = match.confidence;

    sink_->report(detection);
    dispatcher_.dispatch(detection);
}

void ClassificationStage::processWindow() {
    const size_t size = fft_.size();

    for (size_t n = 0; n < size; ++n) {
        fftBuffer_[n] *= windowCoefficients_[n];
    }
    fft_.transform(fftBuffer_.data());

    const size_t half = size / 2;
    for (size_t index = 0; index < size; ++index) {
        // FFT-shift while reading -- see SpectrumStage.cpp's identical
        // comment for why: bin (index + half) % size maps index 0 to
        // the most negative frequency and index size-1 to the most
        // positive.
        const size_t bin = (index + half) % size;
        const std::complex<float>& value = fftBuffer_[bin];
        const float power =
            (value.real() * value.real() + value.imag() * value.imag()) * powerNormalization_;
        magnitudesDb_[index] = 10.0f * std::log10(std::max(power, kPowerFloor));
    }

    const float threshold = estimateNoiseFloorDb() + config_.detectionMarginDb;
    const double spanHz = static_cast<double>(config_.rfSampleRateHz);
    const double binWidthHz = spanHz / static_cast<double>(size);
    const size_t minRunBins = static_cast<size_t>(std::max(1, config_.minRunBins));

    // One pass over the bins, tracking runs of consecutive
    // above-threshold bins. Iterating one past the end with `above`
    // forced false there closes a run that extends all the way to the
    // last bin without a separate check after the loop.
    size_t runStart = 0;
    bool inRun = false;
    for (size_t i = 0; i <= size; ++i) {
        const bool above = (i < size) && (magnitudesDb_[i] > threshold);
        if (above && !inRun) {
            runStart = i;
            inRun = true;
        } else if (!above && inRun) {
            inRun = false;
            if (i - runStart >= minRunBins) emitDetection(runStart, i, binWidthHz);
        }
    }
}

void ClassificationStage::flush() { sink_->flush(); }

void ClassificationStage::reset() {
    fill_ = 0;
    collecting_ = false;
    lastFrameTime_ = std::chrono::steady_clock::time_point{};
}
