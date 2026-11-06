#pragma once

#include <complex>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "core/Config.h"
#include "rf/RtlSdrSource.h"

// Everything needed to receive from an RTL-SDR and turn raw bytes into
// normalized IQ samples: opening the device, tuning it, setting gain,
// and converting the wire format to std::complex<float>.
//
// The byte-to-complex<float> conversion happens here rather than being
// left to whichever stages consume the data, for three reasons: it runs
// once per buffer no matter how many handlers are listening (cheaper
// than N stages each re-deriving it); it keeps every handler's code
// free of the raw-byte normalization logic; and std::complex<float> is
// what FFT/DSP code and libraries overwhelmingly expect as input, so
// producing it centrally means every future stage gets it for free.
//
// Receiver deliberately knows nothing about Dispatcher or message
// types -- it just receives. Samples go to whatever SampleCallback was
// handed to the constructor, invoked synchronously and directly from
// onRawSamples on the RTL-SDR library's own async-read thread. Wiring
// that callback up to Dispatcher::dispatch<IQMessage>() is Toaster's
// job (see core/Toaster.cpp) -- Toaster is the one place responsible
// for connecting components, per Toaster.h. Routing the call through a
// callback instead of dispatching directly here doesn't cost anything:
// it's still one synchronous function call on the same thread, so
// there's no additional risk of falling behind on samples. The actual
// risk of falling behind comes from what a handler does with a buffer
// once it has it, which is unaffected by which class holds the
// Dispatcher reference.
//
// ## Retuning
//
// retune() cannot simply call rtlsdr_set_center_freq() directly on
// whatever thread calls it, even though that thread is usually the RF
// read thread and even though many SDR tools do retune "live" during
// streaming without issue. The distinction that matters: onRawSamples
// (and anything called synchronously from it, including a naive
// retune()) runs *inside* librtlsdr's own async event-processing loop
// -- rtlsdr_read_async() is a blocking call that repeatedly services
// libusb's event loop and invokes onRawSamples directly from within
// that loop, on the same thread, for each completed transfer.
// rtlsdr_set_center_freq() issues its own *synchronous, blocking* USB
// control transfer to reach the tuner over I2C. Calling that
// synchronous transfer reentrantly, from within the very callback the
// event loop is in the middle of servicing, is a different hazard than
// ordinary cross-thread concurrency -- it doesn't need two threads to
// go wrong, just one thread re-entering libusb's event machinery while
// it's already partway through an iteration of it. In practice this
// showed up as intermittent I2C failures during a sweep
// ("rtlsdr_demod_write_reg failed", "r82xx_set_freq: failed") that got
// worse the faster the sweep retuned -- exactly what reentrant misuse
// of the same libusb context looks like, and exactly why a first
// version of this class (which called rtlsdr_set_center_freq() directly
// from retune(), reasoning only about cross-thread safety) broke on
// real hardware despite compiling and working fine against a stub.
//
// The fix is a dedicated tuner thread, started in the constructor and
// joined in the destructor, whose only job is calling
// source_.setCenterFreq(). retune() never touches the device: it just
// records the latest requested frequency under tunerMutex_ and
// notifies tunerCv_, then returns immediately. That's what makes it
// safe to call from the RF read thread (today's only caller) *and* from
// any other thread a future human-typed "tune" console command might
// run on -- the actual USB transfer never happens on the thread that
// called retune(), so there's no reentrancy hazard and no ambiguity
// about which thread needs which synchronization.
class Receiver {
public:
    // Invoked synchronously, once per buffer, on the RTL-SDR async read
    // thread. `samples` points into a buffer Receiver reuses on the next
    // call -- copy anything you need to keep past the callback.
    using SampleCallback = std::function<void(const std::complex<float>* samples, size_t count)>;

    Receiver(const ReceiverConfig& config, SampleCallback onSamples);
    ~Receiver();

    // Non-copyable, non-movable: owns a background thread (see below)
    // and a device handle, neither of which this program ever needs to
    // duplicate or relocate.
    Receiver(const Receiver&) = delete;
    Receiver& operator=(const Receiver&) = delete;

    // Opens and configures the device (sample rate, frequency, gain).
    // Returns false on failure.
    bool open();

    // Streams IQMessages through the Dispatcher until stop() is called.
    // Blocks the calling thread.
    //
    // onStreamingStarted is invoked exactly once, the moment the first
    // real sample buffer arrives -- the earliest point at which stop()
    // is guaranteed to actually take effect, since rtlsdr_cancel_async()
    // is a no-op if the device isn't already confirmed streaming.
    // Starting anything that might call stop() any earlier (e.g. a
    // console listener) is a race: a "stop" typed fast enough would be
    // silently swallowed with no way to retry.
    void run(std::function<void()> onStreamingStarted);

    // Safe to call from any thread; unblocks run().
    void stop();

    // Requests a live center-frequency change. Safe to call from *any*
    // thread, including the RF read thread itself -- returns
    // immediately without touching the device; the actual
    // rtlsdr_set_center_freq() call happens on a dedicated background
    // thread (see the class comment's "Retuning" section for why that
    // indirection is required, not just a nice-to-have). If called
    // again before the previous request has been serviced, only the
    // latest frequency wins -- there's no value in actually tuning
    // through an intermediate frequency nobody asked to stay at.
    void retune(double freqHz);

    static int getDeviceCount();
    static std::string getDeviceName(int index);

private:
    void onRawSamples(const uint8_t* buf, uint32_t len);

    // Runs on tunerThread_ for the Receiver's whole lifetime, from
    // construction to destruction: waits for a pending retune request
    // or a shutdown signal, and calls source_.setCenterFreq() when one
    // arrives. See the class comment's "Retuning" section for why this
    // exists as a separate thread at all.
    void tunerThreadMain();

    ReceiverConfig config_;
    RtlSdrSource source_;
    SampleCallback onSamples_;
    std::vector<std::complex<float>> scratchBuffer_;  // reused across calls, avoids per-buffer allocation
    std::function<void()> onStreamingStarted_;
    std::once_flag streamingStartedFlag_;

    std::thread tunerThread_;
    std::mutex tunerMutex_;
    std::condition_variable tunerCv_;
    bool tunerHasPendingFreq_ = false;
    double tunerPendingFreqHz_ = 0.0;
    bool tunerShouldStop_ = false;
};
