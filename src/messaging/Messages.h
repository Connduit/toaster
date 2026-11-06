#pragma once

#include <complex>
#include <cstddef>
#include <string>

#include "messaging/InternalMessageType.h"

// Each message type is a small, non-owning view over data that already
// lives somewhere else (a buffer owned by Receiver, for instance) plus a
// kType tag Dispatcher uses to route it to the right handlers. Handlers
// receive these by const reference and should copy anything they need
// to keep past the call -- for IQMessage/AudioMessage specifically, the
// underlying samples are only guaranteed valid for the duration of the
// dispatch (Receiver reuses its buffer on the next call, rather than
// allocating a new one every time).

// Raw IQ samples straight from the receiver, already converted from the
// RTL-SDR's raw uint8 wire format to normalized std::complex<float>.
struct IQMessage {
    static constexpr InternalMessageType kType = InternalMessageType::RawIQ;
    const std::complex<float>* samples;
    size_t count;
};

// Post-demodulation float audio samples. Nothing produces this yet --
// reserved for a future stage that wants audio instead of raw IQ (e.g.
// a classifier that cares about demodulated content, not RF spectrum).
struct AudioMessage {
    static constexpr InternalMessageType kType = InternalMessageType::DemodulatedAudio;
    const float* samples;
    size_t count;
};

// A control command, e.g. "stop", "zoom in", "tune 100300000". Small
// and infrequent enough that owning a std::string (rather than a
// non-owning view) is simplest.
//
// One flat command bus rather than a message type per command: every
// interested party (Controller for "stop"/"tune", SpectrumStage for
// "zoom"/"center", possibly others later) subscribes to the same
// CommandMessage and parses out whatever it recognizes, ignoring the
// rest. That's what lets a producer as far removed as ScannerStage
// dispatch "tune <hz>" without needing to know that Controller (which
// retunes the hardware), SpectrumStage, and ClassificationStage (which
// both just want to relabel/reset around the new frequency) are all
// listening -- or that they exist at all.
struct CommandMessage {
    static constexpr InternalMessageType kType = InternalMessageType::Command;
    std::string name;
};

// One feature-based signal detection, published by ClassificationStage
// once per signal it finds in a completed analysis window (see
// classification/ClassificationStage.h). There is no machine learning
// here: centerFreqHz/bandwidthHz/peakDb are measured directly from the
// spectrum, and label/confidence come from matching those against a
// small built-in table of known allocations (see
// classification/DeviceCatalog.h).
//
// Unlike IQMessage/AudioMessage, `label` is safe to keep past the
// dispatch call: it always points at either a string literal ("unknown")
// or an entry in the built-in catalog, both of which have static storage
// duration for the life of the program.
struct SignalDetectionMessage {
    static constexpr InternalMessageType kType = InternalMessageType::SignalDetection;
    double centerFreqHz;
    double bandwidthHz;
    float peakDb;
    const char* label;  // e.g. "FM broadcast", "Wi-Fi (2.4 GHz)", "unknown"
    float confidence;   // 0 (no match found) to 1 (a textbook fit)
};

// Published by ScannerStage whenever it locks onto a frequency it
// judges busy (locked = true) or resumes sweeping past one
// (locked = false, freqHz/powerDb describing the frequency just left).
// Anything that wants to react to scan activity -- a future alerting
// stage, say -- subscribes to this rather than ScannerStage needing to
// know it exists.
struct ScanEventMessage {
    static constexpr InternalMessageType kType = InternalMessageType::ScanEvent;
    double freqHz;
    float powerDb;
    bool locked;
};
