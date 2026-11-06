#include "scanner/ScannerStage.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>

namespace {
constexpr std::chrono::seconds kHeartbeatInterval{5};
constexpr double kMinStepHz = 1.0;  // defensive floor; main.cpp validates the real minimum
}  // namespace

ScannerStage::ScannerStage(const ScannerStageConfig& config, Dispatcher& dispatcher,
                           std::unique_ptr<ScannerSink> sink)
    : config_(config),
      dispatcher_(dispatcher),
      sink_(std::move(sink)),
      currentFreqHz_(config.startFreqHz),
      coarseFreqHz_(config.startFreqHz) {
    if (config_.coarseStepHz < kMinStepHz) config_.coarseStepHz = kMinStepHz;
    if (config_.fineStepHz < 0.0) config_.fineStepHz = 0.0;
    dispatcher.registerTypedHandler<IQMessage>(
        [this](const IQMessage& message) { onIQMessage(message); });
}

void ScannerStage::retuneTo(double freqHz) {
    dispatcher_.dispatch(CommandMessage{"tune " + std::to_string(static_cast<long long>(freqHz))});
}

void ScannerStage::advanceToNextCoarseFrequency() {
    coarseFreqHz_ += config_.coarseStepHz;
    if (coarseFreqHz_ > config_.endFreqHz) coarseFreqHz_ = config_.startFreqHz;
}

void ScannerStage::beginFineRefinement(double centerHz, float coarsePowerDb) {
    fineCandidates_.clear();
    const double half = config_.fineRangeHz / 2.0;
    for (double f = centerHz - half; f <= centerHz + half + 1e-6; f += config_.fineStepHz) {
        fineCandidates_.push_back(f);
    }
    // Shouldn't happen given main.cpp's validation of fineStepHz/
    // fineRangeHz, but keeps "fineCandidates_ is never empty while
    // mode_ == Fine" true regardless.
    if (fineCandidates_.empty()) fineCandidates_.push_back(centerHz);

    fineIndex_ = 0;
    // Seeded with the coarse hit itself: refinement can only find
    // something at least as good, never worse, than just locking on the
    // coarse hit directly.
    fineBestFreqHz_ = centerHz;
    fineBestPowerDb_ = coarsePowerDb;
    mode_ = SweepMode::Fine;

    currentFreqHz_ = fineCandidates_[fineIndex_];
    retuneTo(currentFreqHz_);
    enterSettling();
}

void ScannerStage::enterSettling() {
    phase_ = Phase::Settling;
    phaseDeadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(config_.settleMs);

    if (std::chrono::steady_clock::now() - lastHeartbeat_ >= kHeartbeatInterval) {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "Scanning... now at %.4f MHz", currentFreqHz_ / 1e6);
        sink_->reportStatus(buffer);
        lastHeartbeat_ = std::chrono::steady_clock::now();
    }
}

void ScannerStage::enterMeasuring() {
    phase_ = Phase::Measuring;
    powerSum_ = 0.0;
    powerCount_ = 0;
    phaseDeadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(config_.measureMs);
}

void ScannerStage::enterLocked(float powerDb) {
    phase_ = Phase::Locked;
    lastLockPowerDb_ = powerDb;
    phaseDeadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(config_.lockMs);

    ScanEventMessage event{currentFreqHz_, powerDb, /*locked=*/true};
    sink_->report(event);
    dispatcher_.dispatch(event);
}

void ScannerStage::resumeSweep() {
    ScanEventMessage event{currentFreqHz_, lastLockPowerDb_, /*locked=*/false};
    sink_->report(event);
    dispatcher_.dispatch(event);

    mode_ = SweepMode::Coarse;
    advanceToNextCoarseFrequency();
    currentFreqHz_ = coarseFreqHz_;
    retuneTo(currentFreqHz_);
    enterSettling();
}

void ScannerStage::handleMeasurementComplete(float powerDb) {
    if (mode_ == SweepMode::Coarse) {
        if (powerDb < config_.thresholdDb) {
            advanceToNextCoarseFrequency();
            currentFreqHz_ = coarseFreqHz_;
            retuneTo(currentFreqHz_);
            enterSettling();
            return;
        }
        // A coarse hit. Refine around it unless fine refinement is
        // disabled (fineStepHz or fineRangeHz == 0), in which case lock
        // exactly where the coarse hit landed.
        if (config_.fineStepHz > 0.0 && config_.fineRangeHz > 0.0) {
            beginFineRefinement(currentFreqHz_, powerDb);
        } else {
            enterLocked(powerDb);
        }
        return;
    }

    // SweepMode::Fine: record this candidate, then either move to the
    // next one or, once they're all measured, lock onto the best.
    if (powerDb > fineBestPowerDb_) {
        fineBestPowerDb_ = powerDb;
        fineBestFreqHz_ = currentFreqHz_;
    }
    ++fineIndex_;
    if (fineIndex_ < fineCandidates_.size()) {
        currentFreqHz_ = fineCandidates_[fineIndex_];
        retuneTo(currentFreqHz_);
        enterSettling();
        return;
    }

    // Every candidate measured. Resume the coarse sweep from one step
    // past wherever this refinement started, once the eventual lock
    // ends -- not from any fine candidate's position.
    mode_ = SweepMode::Coarse;
    if (fineBestFreqHz_ != currentFreqHz_) {
        // The best point wasn't the last one measured -- one more
        // retune is needed before locking. The power there is already
        // known, so the next Settling deadline should go straight to
        // enterLocked() rather than re-measuring (see
        // pendingLockAfterSettle_'s comment).
        currentFreqHz_ = fineBestFreqHz_;
        retuneTo(currentFreqHz_);
        pendingLockAfterSettle_ = true;
        enterSettling();
    } else {
        enterLocked(fineBestPowerDb_);
    }
}

void ScannerStage::onIQMessage(const IQMessage& message) {
    if (!started_) {
        started_ = true;
        retuneTo(currentFreqHz_);
        enterSettling();
        return;  // this buffer still reflects whatever -f originally tuned to; discard it
    }

    if (phase_ == Phase::Measuring) {
        for (size_t n = 0; n < message.count; ++n) {
            const float re = message.samples[n].real();
            const float im = message.samples[n].imag();
            powerSum_ += static_cast<double>(re * re + im * im);
        }
        powerCount_ += message.count;
    }

    if (std::chrono::steady_clock::now() < phaseDeadline_) return;

    switch (phase_) {
        case Phase::Settling:
            if (pendingLockAfterSettle_) {
                pendingLockAfterSettle_ = false;
                enterLocked(fineBestPowerDb_);
            } else {
                enterMeasuring();
            }
            break;

        case Phase::Measuring: {
            float powerDb = -200.0f;
            if (powerCount_ > 0) {
                double meanPower = powerSum_ / static_cast<double>(powerCount_);
                powerDb = 10.0f * std::log10(static_cast<float>(std::max(meanPower, 1e-20)));
            }
            handleMeasurementComplete(powerDb);
            break;
        }

        case Phase::Locked:
            resumeSweep();
            break;
    }
}

void ScannerStage::flush() { sink_->flush(); }

void ScannerStage::reset() {
    phase_ = Phase::Settling;
    mode_ = SweepMode::Coarse;
    started_ = false;
    currentFreqHz_ = config_.startFreqHz;
    coarseFreqHz_ = config_.startFreqHz;
    fineCandidates_.clear();
    fineIndex_ = 0;
    pendingLockAfterSettle_ = false;
    powerSum_ = 0.0;
    powerCount_ = 0;
}
