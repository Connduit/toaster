#include "rf/Receiver.h"

#include <cstdio>
#include <utility>

Receiver::Receiver(const ReceiverConfig& config, SampleCallback onSamples)
    : config_(config), source_(config.deviceIndex), onSamples_(std::move(onSamples)) {
    tunerThread_ = std::thread(&Receiver::tunerThreadMain, this);
}

Receiver::~Receiver() {
    {
        std::lock_guard<std::mutex> lock(tunerMutex_);
        tunerShouldStop_ = true;
    }
    tunerCv_.notify_all();
    if (tunerThread_.joinable()) tunerThread_.join();
}

bool Receiver::open() {
    if (!source_.open()) return false;

    source_.setSampleRate(config_.sampleRateHz);
    source_.setCenterFreq(static_cast<uint32_t>(config_.centerFreqHz));

    if (config_.manualGainTenthsDb < 0) {
        source_.setAutoGain();
    } else {
        source_.setManualGain(config_.manualGainTenthsDb);
    }
    return true;
}

void Receiver::run(std::function<void()> onStreamingStarted) {
    onStreamingStarted_ = std::move(onStreamingStarted);
    source_.startAsync([this](const uint8_t* buf, uint32_t len) { onRawSamples(buf, len); });
}

void Receiver::onRawSamples(const uint8_t* buf, uint32_t len) {
    std::call_once(streamingStartedFlag_, [this]() {
        if (onStreamingStarted_) onStreamingStarted_();
    });

    size_t count = len / 2;
    if (scratchBuffer_.size() != count) scratchBuffer_.resize(count);
    for (size_t n = 0; n < count; ++n) {
        float i = (static_cast<float>(buf[2 * n]) - 127.5f) / 127.5f;
        float q = (static_cast<float>(buf[2 * n + 1]) - 127.5f) / 127.5f;
        scratchBuffer_[n] = std::complex<float>(i, q);
    }

    onSamples_(scratchBuffer_.data(), scratchBuffer_.size());
}

void Receiver::stop() { source_.stop(); }

void Receiver::retune(double freqHz) {
    {
        std::lock_guard<std::mutex> lock(tunerMutex_);
        tunerPendingFreqHz_ = freqHz;
        tunerHasPendingFreq_ = true;
    }
    tunerCv_.notify_one();
}

void Receiver::tunerThreadMain() {
    std::unique_lock<std::mutex> lock(tunerMutex_);
    while (!tunerShouldStop_) {
        tunerCv_.wait(lock, [this] { return tunerHasPendingFreq_ || tunerShouldStop_; });
        if (tunerShouldStop_) break;

        double freqHz = tunerPendingFreqHz_;
        tunerHasPendingFreq_ = false;

        // The actual blocking USB control transfer happens here, on
        // this dedicated thread -- never on the RTL-SDR library's own
        // async-read/event-processing thread. See this class's header
        // comment ("Retuning") for why that distinction is required.
        lock.unlock();
        config_.centerFreqHz = freqHz;
        source_.setCenterFreq(static_cast<uint32_t>(freqHz));
        lock.lock();
    }
}

int Receiver::getDeviceCount() { return RtlSdrSource::getDeviceCount(); }

std::string Receiver::getDeviceName(int index) { return RtlSdrSource::getDeviceName(index); }
