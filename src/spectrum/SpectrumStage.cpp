#include "spectrum/SpectrumStage.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "dsp/WindowFunction.h"
#include "spectrum/SpectrumCommands.h"

namespace {

// Power floor before taking a logarithm, so an exactly-zero bin (which
// does happen -- a silent window of all-zero samples produces one)
// yields -200 dB rather than negative infinity. Renderers clamp to the
// configured scale anyway; this exists so no NaN or infinity ever
// reaches them in the first place.
constexpr float kPowerFloor = 1e-20f;

// How long command feedback stays on the header before the standard
// readout takes the space back.
constexpr std::chrono::seconds kStatusLifetime{5};

}  // namespace

SpectrumStage::SpectrumStage(const SpectrumStageConfig& config, Dispatcher& dispatcher,
                              std::unique_ptr<SpectrumSink> sink)
    : config_(config),
      fft_(static_cast<size_t>(config.fftSize > 0 ? config.fftSize : 1)),
      downConverter_(static_cast<double>(config.rfSampleRateHz)),
      sink_(std::move(sink)) {
    // Fft may have rounded the requested size up to a power of two, so
    // every buffer here is sized from what it actually gave us rather
    // than from what the config asked for.
    const size_t size = fft_.size();

    windowCoefficients_ = window::makeHann(size);
    const double windowSum = window::coherentGainSum(windowCoefficients_);
    powerNormalization_ = static_cast<float>(1.0 / (windowSum * windowSum));

    fftBuffer_.resize(size);
    averagedPower_.assign(size, 0.0f);
    magnitudesDb_.assign(size, config_.minDb);

    // Epoch, so the very first window collected is immediately due and
    // the display appears as soon as samples start arriving rather than
    // one refresh interval later.
    lastFrameTime_ = std::chrono::steady_clock::time_point{};

    dispatcher.registerTypedHandler<IQMessage>(
        [this](const IQMessage& message) { onIQMessage(message); });

    // Subscribing to commands the same way Controller does. Neither
    // knows about the other: "stop" is ignored here and "zoom in" is
    // ignored there, and both arrive by the same route.
    dispatcher.registerTypedHandler<CommandMessage>(
        [this](const CommandMessage& command) { onCommand(command); });
}

void SpectrumStage::onCommand(const CommandMessage& command) {
    if (command.name.rfind("tune ", 0) == 0) {
        // Not part of the zoom/pan grammar spectrum_commands owns --
        // this is Controller's retune command (see Controller.cpp),
        // reaching this stage the same broadcast way "stop" reaches
        // Controller. Only centerFreqHz and the stale average need to
        // change; offsetHz/decimation describe the view *relative* to
        // whatever's tuned, so they stay exactly as they were.
        std::string argument = command.name.substr(std::string("tune ").size());
        double freqHz = 0.0;
        bool isRelative = false;
        if (spectrum_commands::parseFrequency(argument, freqHz, isRelative) && !isRelative) {
            std::lock_guard<std::mutex> lock(viewMutex_);
            requestedCenterFreqHz_ = freqHz;
            tuneRequestPending_ = true;
        }
        return;
    }

    const auto result = spectrum_commands::applyCommand(
        command.name, currentRequestedView(), config_.centerFreqHz,
        static_cast<double>(config_.rfSampleRateHz), config_.maxDecimation);
    if (!result.recognized) return;

    std::lock_guard<std::mutex> lock(viewMutex_);
    statusMessage_ = result.message;
    statusSetAt_ = std::chrono::steady_clock::now();
    if (result.applied) {
        requestedView_ = result.view;
        viewRequestPending_ = true;
    }
}

SpectrumView SpectrumStage::currentRequestedView() {
    // Commands compose against the last *requested* view, not the last
    // applied one. Typing "zoom in" twice quickly should zoom twice,
    // even if the RF thread hasn't picked the first one up yet.
    std::lock_guard<std::mutex> lock(viewMutex_);
    return viewRequestPending_ ? requestedView_ : view_;
}

void SpectrumStage::applyPendingView() {
    SpectrumView adopted;
    bool viewPending = false;
    double adoptedCenterFreqHz = config_.centerFreqHz;
    bool tunePending = false;
    {
        std::lock_guard<std::mutex> lock(viewMutex_);
        viewPending = viewRequestPending_;
        if (viewPending) {
            adopted = requestedView_;
            viewRequestPending_ = false;
        }
        tunePending = tuneRequestPending_;
        if (tunePending) {
            adoptedCenterFreqHz = requestedCenterFreqHz_;
            tuneRequestPending_ = false;
        }
    }
    if (!viewPending && !tunePending) return;

    if (viewPending) view_ = adopted;
    config_.centerFreqHz = adoptedCenterFreqHz;
    downConverter_.configure(view_.effectiveOffsetHz(static_cast<double>(config_.rfSampleRateHz)),
                              view_.decimation);

    // The averaged spectrum describes the old slice (or the old
    // frequency entirely, after a retune) at the old resolution;
    // carrying it forward would smear stale data over the first few
    // frames of the new one.
    haveAverage_ = false;
}

void SpectrumStage::beginWindow() {
    collecting_ = true;
    fill_ = 0;
    downConverter_.reset();
    warmUpRemaining_ = downConverter_.isPassThrough() ? 0 : downConverter_.warmUpSamples();
}

bool SpectrumStage::fillWindow(const std::complex<float>* samples, size_t count) {
    size_t offset = 0;

    // The cascade's first outputs reflect empty filter history rather
    // than the signal, so they're dropped before anything is kept.
    if (warmUpRemaining_ > 0) {
        const size_t dropped = std::min(warmUpRemaining_, count);
        warmUpRemaining_ -= dropped;
        offset += dropped;
    }

    const size_t size = fft_.size();
    const size_t take = std::min(size - fill_, count - offset);
    std::copy(samples + offset, samples + offset + take,
               fftBuffer_.begin() + static_cast<std::ptrdiff_t>(fill_));
    fill_ += take;
    return fill_ == size;
}

bool SpectrumStage::frameDue() const {
    const auto elapsed = std::chrono::steady_clock::now() - lastFrameTime_;
    return elapsed >= std::chrono::milliseconds(config_.refreshIntervalMs);
}

void SpectrumStage::onIQMessage(const IQMessage& message) {
    if (!collecting_) {
        // The clock is read here, once per buffer rather than once per
        // sample -- at 1.92 MS/s a per-sample steady_clock call would
        // cost more than the FFT it exists to avoid.
        if (!frameDue()) return;

        // A window boundary is the only safe moment to change the view:
        // nothing is half-collected, so rebuilding the downconverter
        // can't corrupt a measurement in progress.
        applyPendingView();
        beginWindow();
    }

    bool complete = false;
    if (downConverter_.isPassThrough()) {
        // Unzoomed, there's nothing to mix or filter, so the samples go
        // straight into the window. Worth the branch: this is the
        // default view, and it keeps the common case entirely free of
        // the downconverter's cost.
        complete = fillWindow(message.samples, message.count);
    } else {
        decimated_.clear();
        downConverter_.process(message.samples, message.count, decimated_);
        complete = fillWindow(decimated_.data(), decimated_.size());
    }

    if (!complete) return;  // window spans more messages; carry on next time

    processWindow();
    collecting_ = false;
    fill_ = 0;
    lastFrameTime_ = std::chrono::steady_clock::now();
    // Whatever remains of this buffer is deliberately dropped: the
    // stage is idle again until the refresh interval elapses.
}

void SpectrumStage::processWindow() {
    const size_t size = fft_.size();

    // Taper the block to zero at both ends before transforming. See
    // dsp/WindowFunction.h for why this is not optional: without it, a
    // strong local station smears across the entire display and buries
    // everything weaker.
    for (size_t n = 0; n < size; ++n) {
        fftBuffer_[n] *= windowCoefficients_[n];
    }

    fft_.transform(fftBuffer_.data());

    const float newWeight = std::min(1.0f, std::max(0.0f, config_.smoothingFactor));
    const float oldWeight = 1.0f - newWeight;
    const size_t half = size / 2;

    for (size_t index = 0; index < size; ++index) {
        // FFT shift, done while reading rather than as a separate pass:
        // the transform puts negative frequencies in the upper half of
        // the array, but a display wants them on the left. Output index
        // `index` therefore reads bin (index + size/2) mod size, which
        // maps index 0 to the most negative frequency and index size-1
        // to the most positive.
        const size_t bin = (index + half) % size;

        const std::complex<float>& value = fftBuffer_[bin];
        const float power =
            (value.real() * value.real() + value.imag() * value.imag()) * powerNormalization_;

        // Average in linear power, not in dB. Averaging dB values
        // averages logarithms, which is a geometric mean -- it biases
        // low, and biases further the noisier the bin, so it would make
        // the noise floor read lower than it actually is. Averaging
        // power and then taking the log gives the arithmetic mean,
        // which is what "average power" means and what reduces the
        // variance of a noisy bin correctly.
        //
        // A plain exponential moving average needs a starting value;
        // seeding it with the first real frame rather than with zero
        // avoids the display climbing up from the floor over the first
        // second or so.
        const float averaged =
            haveAverage_ ? newWeight * power + oldWeight * averagedPower_[index] : power;
        averagedPower_[index] = averaged;

        magnitudesDb_[index] = 10.0f * std::log10(std::max(averaged, kPowerFloor));
    }
    haveAverage_ = true;

    SpectrumFrame frame;
    frame.magnitudesDb = magnitudesDb_.data();
    frame.binCount = magnitudesDb_.size();
    frame.centerFreqHz =
        view_.centerHz(config_.centerFreqHz, static_cast<double>(config_.rfSampleRateHz));
    // A complex (IQ) stream sampled at fs carries a full fs of
    // bandwidth, not fs/2: negative frequencies are distinguishable
    // from positive ones rather than mirrored, as they would be for a
    // real-valued signal. Decimating by D narrows that proportionally.
    frame.spanHz = view_.spanHz(static_cast<double>(config_.rfSampleRateHz));
    frame.minDb = config_.minDb;
    frame.maxDb = config_.maxDb;
    frame.zoom = view_.decimation;

    // Copied out under the lock so the renderer reads a stable string
    // rather than one the console thread might be assigning to.
    std::string status;
    {
        std::lock_guard<std::mutex> lock(viewMutex_);
        if (!statusMessage_.empty() &&
            std::chrono::steady_clock::now() - statusSetAt_ > kStatusLifetime) {
            statusMessage_.clear();
        }
        status = statusMessage_;
    }
    frame.status = status.empty() ? nullptr : status.c_str();

    sink_->renderFrame(frame);
}

void SpectrumStage::flush() { sink_->flush(); }

void SpectrumStage::reset() {
    fill_ = 0;
    collecting_ = false;
    warmUpRemaining_ = 0;
    downConverter_.reset();
    haveAverage_ = false;
    std::fill(averagedPower_.begin(), averagedPower_.end(), 0.0f);
    std::fill(magnitudesDb_.begin(), magnitudesDb_.end(), config_.minDb);
    lastFrameTime_ = std::chrono::steady_clock::time_point{};
}
