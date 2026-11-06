#include "classification/DeviceCatalog.h"

#include <algorithm>
#include <cmath>

namespace classification {

namespace {

// How well a measured bandwidth fits a profile's typical range, in
// (0, 1]. 1.0 for squarely inside the range. Outside it, decays
// smoothly rather than dropping to zero at the boundary: a Wi-Fi
// channel bonded wider than typical, say, is still very plausibly
// Wi-Fi, just a worse fit than a textbook one. Never returns exactly 0
// for a frequency-range match, by design -- matchDevice() below is what
// decides "unknown" (no candidate at all), not this function.
float scoreBandwidthFit(double bandwidthHz, double minBandwidthHz, double maxBandwidthHz) {
    if (bandwidthHz >= minBandwidthHz && bandwidthHz <= maxBandwidthHz) return 1.0f;
    if (bandwidthHz < minBandwidthHz) {
        if (minBandwidthHz <= 0.0) return 0.0f;
        return static_cast<float>(std::clamp(bandwidthHz / minBandwidthHz, 0.0, 1.0));
    }
    if (bandwidthHz <= 0.0) return 0.0f;
    return static_cast<float>(std::clamp(maxBandwidthHz / bandwidthHz, 0.0, 1.0));
}

}  // namespace

const std::vector<DeviceProfile>& builtInDeviceCatalog() {
    // Center frequencies/bandwidths are well-documented public
    // allocations (ITU/FCC/ETSI), not anything specific to a particular
    // device. Whether a given dongle can actually tune high enough to
    // reach the 2.4 GHz entries depends on its tuner chip (many common
    // RTL-SDR tuners top out well below that); entries here don't
    // assume any particular hardware ceiling.
    static const std::vector<DeviceProfile> catalog = {
        {"FM broadcast", 88.0e6, 108.0e6, 150e3, 200e3},
        {"NOAA weather radio", 162.400e6, 162.550e6, 11e3, 25e3},
        {"Aviation VHF voice (AM)", 118.0e6, 137.0e6, 8e3, 25e3},
        {"Marine VHF", 156.0e6, 162.025e6, 12.5e3, 25e3},
        {"Amateur radio (2 m band)", 144.0e6, 148.0e6, 2.5e3, 16e3},
        {"Amateur radio (70 cm band)", 420.0e6, 450.0e6, 2.5e3, 16e3},
        {"ISM 433 MHz (short-range devices)", 433.05e6, 434.79e6, 10e3, 150e3},
        {"FRS/GMRS", 462.5e6, 467.7e6, 12.5e3, 25e3},
        {"ISM 868 MHz (short-range devices / LoRa)", 863.0e6, 870.0e6, 12.5e3, 500e3},
        {"ISM 915 MHz (short-range devices / LoRa)", 902.0e6, 928.0e6, 12.5e3, 500e3},
        {"DECT cordless phone", 1880.0e6, 1900.0e6, 1.0e6, 1.8e6},
        {"Wi-Fi (2.4 GHz)", 2400.0e6, 2483.5e6, 20.0e6, 40.0e6},
        {"Bluetooth", 2400.0e6, 2483.5e6, 1.0e6, 2.0e6},
        {"Zigbee (2.4 GHz)", 2400.0e6, 2483.5e6, 2.0e6, 3.0e6},
    };
    return catalog;
}

MatchResult matchDevice(double centerFreqHz, double bandwidthHz,
                         const std::vector<DeviceProfile>& catalog) {
    MatchResult best;
    for (const DeviceProfile& profile : catalog) {
        if (centerFreqHz < profile.minFreqHz || centerFreqHz > profile.maxFreqHz) continue;
        float confidence = scoreBandwidthFit(bandwidthHz, profile.minBandwidthHz, profile.maxBandwidthHz);
        if (confidence > best.confidence) {
            best.label = profile.name;
            best.confidence = confidence;
        }
    }
    return best;
}

}  // namespace classification
