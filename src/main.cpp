// rtlsdr_fm (aka "Toaster") - Minimal wideband-FM broadcast receiver for
// RTL-SDR.
//
// See README.md for build instructions and usage.
//
// Usage: rtlsdr_fm -f <freq_hz> [-d <device_index>] [-g <gain_db|auto>] [-o <file.wav>]
//                  [-t fir|iir] [-s <stage,...>] [-v bars|waterfall] [-n <fft_size>]
//                  [-r <rf_sample_rate_hz>] [scanner/classification flags -- see printUsage() below]
// Examples:
//   ./rtlsdr_fm -f 100100000 | aplay -r 48000 -f S16_LE -c 1
//   ./rtlsdr_fm -f 100100000 -o station.wav
//   ./rtlsdr_fm -f 100100000 -t iir | aplay -r 48000 -f S16_LE -c 1
//   ./rtlsdr_fm -f 100100000 -s spectrum-visual -v waterfall
//   ./rtlsdr_fm -f 100100000 -s spectrum-visual -r 2400000
//   ./rtlsdr_fm -f 100100000 -s audio,scanner,classification --scan-start 88M --scan-end 108M
//
// While running, type "stop" and press Enter to end the program cleanly
// (this flushes buffers and, in .wav mode, finalizes the file header).
// Ctrl+C is intentionally not handled: cancelling an in-flight USB read
// from a signal handler is unsafe, so "stop" is the supported way to end
// the program.

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

#include "core/Toaster.h"
#include "spectrum/SpectrumCommands.h"

namespace {

void printUsage(const char* prog) {
    fprintf(stderr,
        "Usage: %s -f <freq_hz> [-d <device_index>] [-g <gain_db|auto>] [-o <file.wav>]\n"
        "          [-t fir|iir] [-s <stage,stage,...>] [-v bars|waterfall] [-n <fft_size>]\n"
        "          [-r <rf_sample_rate_hz>] [--scan-* ...] [--classification-* ...]\n"
        "Examples:\n"
        "  %s -f 100100000 | aplay -r 48000 -f S16_LE -c 1\n"
        "  %s -f 100100000 -o station.wav\n"
        "  %s -f 100100000 -t iir | aplay -r 48000 -f S16_LE -c 1\n"
        "  %s -f 100100000 -s spectrum-visual\n"
        "  %s -f 100100000 -s spectrum-visual -v waterfall -n 2048\n"
        "  %s -f 100100000 -s audio,spectrum-visual | aplay -r 48000 -f S16_LE -c 1\n"
        "  %s -f 100100000 -s spectrum-visual -r 2400000\n"
        "  %s -f 100100000 -s audio,scanner,classification --scan-start 88M --scan-end 108M \\\n"
        "     | aplay -r 48000 -f S16_LE -c 1\n"
        "\n"
        "-t selects the anti-aliasing filter implementation (default: fir).\n"
        "-s selects which pipeline stage(s) to run, comma-separated (e.g.\n"
        "   \"audio\", \"spectrum-visual\", \"classification\", \"scanner\", or any\n"
        "   combination of them). If omitted, you'll be prompted for this on\n"
        "   the console at startup.\n"
        "-v selects the spectrum display style (default: bars). Only relevant\n"
        "   when the spectrum-visual stage is running.\n"
        "-n sets the spectrum-visual stage's FFT size in bins; must be a power\n"
        "   of two (default: 1024). Larger resolves finer detail in frequency\n"
        "   but averages over a longer stretch of time.\n"
        "-r sets how much RF bandwidth is captured, in Hz (default:\n"
        "   1920000, i.e. 1.92 MHz). This is also the widest span \"zoom\n"
        "   out\"/\"zoom reset\" can ever reach on the spectrum display,\n"
        "   since the display can only ever show a slice of what was\n"
        "   actually captured -- it never retunes the dongle (see the\n"
        "   README's \"Zooming and panning\" section). Must be a rate\n"
        "   librtlsdr's RTL2832U supports: 225001-300000 or\n"
        "   900001-3200000. Above ~2.4M, USB throughput on some hosts\n"
        "   can't keep up and samples get dropped; above 2.88M this is\n"
        "   common enough to be worth testing before relying on it.\n"
        "\n"
        "Scanner (only relevant when the \"scanner\" stage is running -- it's\n"
        "the one stage here that retunes the hardware; see scanner/\n"
        "ScannerStage.h). Frequencies accept the same forms as -f/-r suffixed\n"
        "with k/M/G, e.g. \"88M\". Sweeping is coarse-then-fine: a wide coarse\n"
        "pass finds candidates, then each one gets a finer sub-sweep to\n"
        "localize its actual peak before locking:\n"
        "  --scan-start <hz>       sweep range start (default: 88M)\n"
        "  --scan-end <hz>         sweep range end (default: 108M)\n"
        "  --scan-step <hz>        coarse survey step (default: 500k)\n"
        "  --scan-fine-step <hz>   refinement step around a coarse hit (default:\n"
        "                          25k); 0 disables refinement and locks exactly\n"
        "                          on the coarse hit\n"
        "  --scan-fine-range <hz>  how wide a neighborhood around a coarse hit\n"
        "                          gets refined (default: 500k, matching the\n"
        "                          default coarse step); 0 also disables\n"
        "                          refinement\n"
        "  --scan-threshold <db>   RSSI level, in dBFS, considered \"busy\" (default: -30).\n"
        "                          Depends heavily on -g's gain setting; if the scanner\n"
        "                          never locks or locks on everything, this is the first\n"
        "                          thing to adjust.\n"
        "  --scan-settle-ms <ms>   discarded after each retune for the tuner's PLL to\n"
        "                          settle (default: 15)\n"
        "  --scan-measure-ms <ms>  RSSI measurement window per channel (default: 20)\n"
        "  --scan-lock-ms <ms>     how long to stay on a hit before resuming, when\n"
        "                          \"audio\" is also selected (default: 8000). If\n"
        "                          \"audio\" ISN'T selected, --scan-quick-confirm-ms is\n"
        "                          used instead unless this flag is passed explicitly\n"
        "                          -- see that flag.\n"
        "  --scan-quick-confirm-ms <ms>  how long to hold a hit when nothing is\n"
        "                          listening live (default: 300, just enough for one\n"
        "                          classification window). Without this, a scan-only\n"
        "                          run over a busy band would spend the full\n"
        "                          --scan-lock-ms on every single hit for no benefit,\n"
        "                          since nothing's there to hear it.\n"
        "  --scan-log <path>       where lock/resume events are written, or \"-\" for\n"
        "                          stderr (default: scan.log)\n"
        "\n"
        "Classification (only relevant when the \"classification\" stage is\n"
        "running; see classification/ClassificationStage.h). Feature-based --\n"
        "no machine learning -- matching against a small built-in table of\n"
        "known allocations (see classification/DeviceCatalog.h):\n"
        "  --classification-fft-size <n>       FFT size in bins, a power of two\n"
        "                                       (default: 1024)\n"
        "  --classification-refresh-ms <ms>    how often a window is analyzed\n"
        "                                       (default: 200)\n"
        "  --classification-margin-db <db>     how far above the noise floor a\n"
        "                                       bin must read to count as signal\n"
        "                                       (default: 12)\n"
        "  --classification-min-run-bins <n>   consecutive above-threshold bins\n"
        "                                       needed to report a detection,\n"
        "                                       rather than dismiss it as a noise\n"
        "                                       spike (default: 3)\n"
        "  --classification-log <path>         where detections are written, or\n"
        "                                       \"-\" for stderr (default:\n"
        "                                       classification.log)\n"
        "\n"
        "classification/scanner default to log files rather than stderr because\n"
        "spectrum-visual's live display also uses stderr and the two will\n"
        "corrupt each other's output if both write there -- Toaster warns at\n"
        "startup if that combination is configured.\n"
        "\n"
        "The spectrum display draws to stderr, never stdout, so it can run\n"
        "alongside audio being piped from stdout into aplay.\n"
        "While running, type 'stop' and press Enter to end the program.\n",
        prog, prog, prog, prog, prog, prog, prog, prog, prog);
}

// Splits a comma-separated list into trimmed tokens, e.g.
// "audio, spectrum-visual" -> {"audio", "spectrum-visual"}. Case and
// validation against the known stages are handled later by
// core/StageSelection.h, once Toaster knows what's actually available.
std::vector<std::string> splitCommaList(const std::string& arg) {
    std::vector<std::string> tokens;
    std::stringstream ss(arg);
    std::string token;
    while (std::getline(ss, token, ',')) {
        size_t start = token.find_first_not_of(" \t");
        size_t end = token.find_last_not_of(" \t");
        if (start == std::string::npos) continue;
        tokens.push_back(token.substr(start, end - start + 1));
    }
    return tokens;
}

// Parses one CLI argument as a frequency (see
// spectrum_commands::parseFrequency for the accepted forms: "100.3M",
// "200k", "7500", etc.), printing a specific complaint and returning
// false on failure rather than falling through to the generic
// printUsage() -- a malformed frequency deserves more than "usage: ...".
// `rejectRelative` matters for a flag naming an absolute frequency
// (scan-start/scan-end): "+100M" as a starting frequency isn't a
// meaningful instruction, the same reasoning -r and "tune" apply.
bool parseFrequencyArg(const char* flagName, const std::string& text, double& hz, bool rejectRelative) {
    bool isRelative = false;
    if (!spectrum_commands::parseFrequency(text, hz, isRelative)) {
        fprintf(stderr, "%s expects a frequency (e.g. \"100.3M\", \"200k\", \"7500\"), got \"%s\".\n",
                flagName, text.c_str());
        return false;
    }
    if (rejectRelative && isRelative) {
        fprintf(stderr, "%s expects an absolute frequency, not a relative one (\"%s\").\n", flagName,
                text.c_str());
        return false;
    }
    return true;
}

// Validates a power-of-two FFT size, shared between -n and
// --classification-fft-size. See -n's own inline comment for why this
// rejects rather than rounds.
bool parseFftSizeArg(const char* flagName, const std::string& text, int& fftSize) {
    fftSize = std::atoi(text.c_str());
    if (fftSize < 16 || (fftSize & (fftSize - 1)) != 0) {
        fprintf(stderr, "%s must be a power of two, at least 16 (e.g. 512, 1024, 2048).\n", flagName);
        return false;
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    ToasterConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-f" && i + 1 < argc) {
            config.centerFreqHz = std::atof(argv[++i]);
        } else if (arg == "-d" && i + 1 < argc) {
            config.deviceIndex = std::atoi(argv[++i]);
        } else if (arg == "-g" && i + 1 < argc) {
            std::string g = argv[++i];
            if (g != "auto") config.manualGainTenthsDb = static_cast<int>(std::atof(g.c_str()) * 10);
        } else if (arg == "-o" && i + 1 < argc) {
            config.outputMode = AudioOutputMode::WavFile;
            config.outputWavPath = argv[++i];
        } else if (arg == "-t" && i + 1 < argc) {
            std::string t = argv[++i];
            if (t == "iir") {
                config.antiAliasFilterType = FilterImplementation::Iir;
            } else if (t == "fir") {
                config.antiAliasFilterType = FilterImplementation::Fir;
            } else {
                printUsage(argv[0]);
                return 1;
            }
        } else if (arg == "-s" && i + 1 < argc) {
            config.requestedStageKeys = splitCommaList(argv[++i]);
        } else if (arg == "-v" && i + 1 < argc) {
            std::string v = argv[++i];
            if (v == "waterfall") {
                config.spectrumRenderer = SpectrumRendererType::Waterfall;
            } else if (v == "bars") {
                config.spectrumRenderer = SpectrumRendererType::Bars;
            } else {
                printUsage(argv[0]);
                return 1;
            }
        } else if (arg == "-n" && i + 1 < argc) {
            if (!parseFftSizeArg("-n", argv[++i], config.spectrumFftSize)) return 1;
        } else if (arg == "-r" && i + 1 < argc) {
            double rate = std::atof(argv[++i]);
            // These bounds are librtlsdr's actual constraints for the
            // RTL2832U (rtlsdr_set_sample_rate rejects anything outside
            // them, and 300001-900000 is a dead zone in between) --
            // rejected here, with an explanation, rather than letting a
            // bad value silently fail deep inside Receiver::open() with
            // no context for why.
            bool inLowRange = rate >= 225001.0 && rate <= 300000.0;
            bool inHighRange = rate >= 900001.0 && rate <= 3200000.0;
            if (!inLowRange && !inHighRange) {
                fprintf(stderr,
                    "-r must be 225001-300000 or 900001-3200000 (RTL2832U's supported\n"
                    "sample rates); %.0f is outside both ranges.\n",
                    rate);
                return 1;
            }
            // Not a hard error: this only matters if the audio stage
            // ends up selected (which isn't known yet -- stage selection
            // can happen interactively, inside Toaster's constructor),
            // and even then it's a quality issue, not a crash. See
            // AudioStage::AudioStage's decimator_ construction, which
            // truncates this ratio to an int.
            const double kAudioSampleRateHz = 48000.0;
            double factor = rate / kAudioSampleRateHz;
            double truncatedFactor = static_cast<double>(static_cast<int>(factor));
            if (truncatedFactor > 0 && factor != truncatedFactor) {
                double actualAudioRate = rate / truncatedFactor;
                fprintf(stderr,
                    "Note: -r %.0f does not divide evenly by %.0f Hz audio. If the audio\n"
                    "stage runs, its actual output rate will be ~%.1f Hz, not %.0f Hz --\n"
                    "a %.2f%% speed/pitch drift. Multiples of %.0f (e.g. %.0f, %.0f, %.0f)\n"
                    "avoid this. Spectrum-only use is unaffected.\n",
                    rate, kAudioSampleRateHz, actualAudioRate, kAudioSampleRateHz,
                    100.0 * (actualAudioRate - kAudioSampleRateHz) / kAudioSampleRateHz,
                    kAudioSampleRateHz, kAudioSampleRateHz * 40, kAudioSampleRateHz * 50,
                    kAudioSampleRateHz * 60);
            }
            if (rate > 2880000.0) {
                fprintf(stderr,
                    "Note: -r %.0f is above the ~2.4-2.88 MS/s range most RTL-SDR dongles\n"
                    "sustain reliably over USB. Watch stderr for dropped-sample symptoms\n"
                    "(choppy audio, spectrum frames stalling) and back off if you see them.\n",
                    rate);
            }
            config.rfSampleRateHz = static_cast<uint32_t>(rate);
        } else if (arg == "--scan-start" && i + 1 < argc) {
            if (!parseFrequencyArg("--scan-start", argv[++i], config.scanStartFreqHz, true)) return 1;
        } else if (arg == "--scan-end" && i + 1 < argc) {
            if (!parseFrequencyArg("--scan-end", argv[++i], config.scanEndFreqHz, true)) return 1;
        } else if (arg == "--scan-step" && i + 1 < argc) {
            if (!parseFrequencyArg("--scan-step", argv[++i], config.scanCoarseStepHz, false)) return 1;
            if (config.scanCoarseStepHz <= 0.0) {
                fprintf(stderr, "--scan-step must be positive.\n");
                return 1;
            }
        } else if (arg == "--scan-fine-step" && i + 1 < argc) {
            if (!parseFrequencyArg("--scan-fine-step", argv[++i], config.scanFineStepHz, false)) return 1;
            if (config.scanFineStepHz < 0.0) {
                fprintf(stderr, "--scan-fine-step must not be negative (0 disables fine refinement).\n");
                return 1;
            }
        } else if (arg == "--scan-fine-range" && i + 1 < argc) {
            if (!parseFrequencyArg("--scan-fine-range", argv[++i], config.scanFineRangeHz, false)) return 1;
            if (config.scanFineRangeHz < 0.0) {
                fprintf(stderr, "--scan-fine-range must not be negative (0 disables fine refinement).\n");
                return 1;
            }
        } else if (arg == "--scan-threshold" && i + 1 < argc) {
            config.scanThresholdDb = static_cast<float>(std::atof(argv[++i]));
        } else if (arg == "--scan-settle-ms" && i + 1 < argc) {
            config.scanSettleMs = std::atoi(argv[++i]);
        } else if (arg == "--scan-measure-ms" && i + 1 < argc) {
            config.scanMeasureMs = std::atoi(argv[++i]);
            if (config.scanMeasureMs < 1) {
                fprintf(stderr, "--scan-measure-ms must be at least 1.\n");
                return 1;
            }
        } else if (arg == "--scan-lock-ms" && i + 1 < argc) {
            config.scanLockMs = std::atoi(argv[++i]);
            if (config.scanLockMs < 0) {
                fprintf(stderr, "--scan-lock-ms must not be negative.\n");
                return 1;
            }
            config.scanLockMsExplicit = true;
        } else if (arg == "--scan-quick-confirm-ms" && i + 1 < argc) {
            config.scanQuickConfirmMs = std::atoi(argv[++i]);
            if (config.scanQuickConfirmMs < 0) {
                fprintf(stderr, "--scan-quick-confirm-ms must not be negative.\n");
                return 1;
            }
        } else if (arg == "--scan-log" && i + 1 < argc) {
            config.scanLogPath = argv[++i];
        } else if (arg == "--classification-log" && i + 1 < argc) {
            config.classificationLogPath = argv[++i];
        } else if (arg == "--classification-fft-size" && i + 1 < argc) {
            if (!parseFftSizeArg("--classification-fft-size", argv[++i], config.classificationFftSize)) {
                return 1;
            }
        } else if (arg == "--classification-refresh-ms" && i + 1 < argc) {
            config.classificationRefreshIntervalMs = std::atoi(argv[++i]);
            if (config.classificationRefreshIntervalMs < 1) {
                fprintf(stderr, "--classification-refresh-ms must be at least 1.\n");
                return 1;
            }
        } else if (arg == "--classification-margin-db" && i + 1 < argc) {
            config.classificationDetectionMarginDb = static_cast<float>(std::atof(argv[++i]));
            if (config.classificationDetectionMarginDb <= 0.0f) {
                fprintf(stderr,
                        "--classification-margin-db must be positive (a signal must read some "
                        "amount above the noise floor to count as one).\n");
                return 1;
            }
        } else if (arg == "--classification-min-run-bins" && i + 1 < argc) {
            config.classificationMinRunBins = std::atoi(argv[++i]);
            if (config.classificationMinRunBins < 1) {
                fprintf(stderr, "--classification-min-run-bins must be at least 1.\n");
                return 1;
            }
        } else {
            printUsage(argv[0]);
            return 1;
        }
    }

    if (config.scanStartFreqHz >= config.scanEndFreqHz) {
        fprintf(stderr, "--scan-start must be less than --scan-end (got %.0f and %.0f).\n",
                config.scanStartFreqHz, config.scanEndFreqHz);
        return 1;
    }

    if (config.centerFreqHz <= 0.0) {
        printUsage(argv[0]);
        return 1;
    }

    // Toaster owns everything from here: the Receiver, the Dispatcher,
    // every stage selected to run (AudioStage, SpectrumStage), the
    // ConsoleCommandListener, and the wiring between all of them. main()
    // just builds it and runs it.
    Toaster toaster(config);
    bool ok = toaster.run();

    fprintf(stderr, "\nDone.\n");
    return ok ? 0 : 1;
}
