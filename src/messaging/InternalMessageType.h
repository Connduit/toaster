#pragma once

// Identifies what kind of data or command a message carries. Adding a
// new stage or command usually means adding a new enumerator here, plus
// a matching struct in Messages.h -- not a new method on Dispatcher,
// which is what keeps Dispatcher itself generic and stable as the
// project grows.
enum class InternalMessageType {
    RawIQ,             // std::complex<float> samples straight from the receiver
    DemodulatedAudio,  // post-demodulation float audio samples (not produced yet;
                       // reserved for a future stage that wants audio instead of IQ)
    Command,           // a control command (e.g. "stop", "tune <hz>") for Controller
                       // and any other interested stage to act on
    SignalDetection,   // a feature-based signal detection + best-guess device match,
                       // published by ClassificationStage
    ScanEvent,         // a lock/resume transition, published by ScannerStage
};
