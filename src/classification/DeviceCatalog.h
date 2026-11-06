#pragma once

#include <string>
#include <vector>

// A small built-in table of known frequency allocations, and a
// feature-based match against it -- no machine learning, by design.
// Kept separate from ClassificationStage so it can be tested, extended,
// or swapped out without touching the stage, the dispatcher, or a
// receiver, the same reasoning spectrum_commands.h gives for keeping
// zoom/pan parsing out of SpectrumStage itself.
//
// "Feature-based" here means exactly two measured features: where a
// signal sits (centerFreqHz) and how wide it is (bandwidthHz), both of
// which ClassificationStage derives directly from a run of FFT bins
// that cross a noise-floor-relative threshold. Matching is a lookup
// against known ranges, not a learned model -- deliberately, for now.
// A real classifier (modulation recognition, RF fingerprinting, an
// actual ML model) is a different, much larger project; this gives
// today's users an honest "here's what this could plausibly be, given
// only its frequency and width" rather than pretending to more
// certainty than a two-number measurement can support.
namespace classification {

// One known allocation. minBandwidthHz/maxBandwidthHz describe the
// range of channel widths actually used within this allocation, not
// the allocation's own total width -- e.g. the Wi-Fi 2.4 GHz band spans
// tens of MHz, but any one Wi-Fi signal is 20-40 MHz wide.
struct DeviceProfile {
    const char* name;
    double minFreqHz;
    double maxFreqHz;
    double minBandwidthHz;
    double maxBandwidthHz;
};

// The result of matching a measured signal against the catalog.
// `label` has static storage duration for the life of the program --
// either a DeviceProfile::name from the catalog, or the literal
// "unknown" -- so it's always safe to keep past the call that produced
// it, unlike the non-owning views in messaging/Messages.h.
struct MatchResult {
    const char* label = "unknown";
    float confidence = 0.0f;  // 0 (no match) to 1 (measured bandwidth is
                               // squarely inside the profile's typical range)
};

// The catalog itself: common ISM/consumer allocations likely to be seen
// with a bare RTL-SDR (no cellular bands, which need a downconverter or
// a different dongle to reach, and no attempt at exhaustiveness --
// adding an entry is the whole job of extending what this recognizes).
// A free function rather than a global so nothing pays static-init
// order cost for a table only classification/ actually uses.
const std::vector<DeviceProfile>& builtInDeviceCatalog();

// Matches a measured (centerFreqHz, bandwidthHz) pair against `catalog`.
//
// Only profiles whose frequency range contains centerFreqHz are
// candidates at all. Among those, confidence comes from how well
// bandwidthHz fits the candidate's typical range: 1.0 for squarely
// inside it, decaying smoothly for a bandwidth that's plausible but
// off, e.g. one Wi-Fi channel bonded into a wider one than typical.
// When more than one candidate's frequency range contains the signal
// (allocations do overlap, e.g. Bluetooth and Wi-Fi both use 2.4 GHz),
// the higher-confidence match wins; a tie keeps whichever the catalog
// lists first.
//
// Returns {"unknown", 0.0} if no profile's frequency range contains
// centerFreqHz at all.
MatchResult matchDevice(double centerFreqHz, double bandwidthHz,
                         const std::vector<DeviceProfile>& catalog = builtInDeviceCatalog());

}  // namespace classification
