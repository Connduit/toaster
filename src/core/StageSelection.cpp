#include "core/StageSelection.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

// Splits a comma-separated list into trimmed, lower-cased, non-empty
// tokens. "audio, spectrum-visual" -> {"audio", "spectrum-visual"}.
std::vector<std::string> splitKeys(const std::string& line) {
    std::vector<std::string> keys;
    std::stringstream ss(line);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token = toLower(trim(token));
        if (!token.empty()) keys.push_back(token);
    }
    return keys;
}

bool isKnownKey(const std::string& key, const std::vector<StageDescriptor>& available) {
    return std::any_of(available.begin(), available.end(),
                        [&](const StageDescriptor& d) { return d.key == key; });
}

// Keeps only the keys that match an available stage, warning (via
// console) about any that don't.
std::vector<std::string> keepValid(const std::vector<std::string>& keys,
                                    const std::vector<StageDescriptor>& available, Console& console) {
    std::vector<std::string> valid;
    for (const auto& key : keys) {
        if (isKnownKey(key, available)) {
            valid.push_back(key);
        } else {
            console.printLine("Unknown stage '" + key + "', ignoring.");
        }
    }
    return valid;
}

void printAvailableStages(Console& console, const std::vector<StageDescriptor>& available) {
    console.printLine("Available stages:");
    for (const auto& d : available) {
        console.printLine("  " + d.key + " - " + d.description);
    }
}

// Prompts on the console until at least one valid stage key is entered,
// or the input stream closes (EOF), in which case the first available
// stage is returned as the default so the program doesn't hang forever
// (e.g. stdin redirected from /dev/null).
std::vector<std::string> promptInteractive(Console& console, const std::vector<StageDescriptor>& available) {
    printAvailableStages(console, available);
    const std::string defaultKey = available.front().key;

    while (true) {
        console.printLine("Enter stage(s) to run, comma-separated (default: " + defaultKey + "):");
        std::string line;
        if (!console.readLine(line)) {
            console.printLine("Input closed; defaulting to '" + defaultKey + "'.");
            return {defaultKey};
        }

        line = trim(line);
        if (line.empty()) return {defaultKey};

        std::vector<std::string> valid = keepValid(splitKeys(line), available, console);
        if (!valid.empty()) return valid;

        console.printLine("No valid stage selected, try again.");
    }
}

}  // namespace

std::vector<std::string> resolveStages(Console& console, const std::vector<StageDescriptor>& available,
                                        const std::vector<std::string>& requested) {
    if (!requested.empty()) {
        std::vector<std::string> valid = keepValid(requested, available, console);
        if (!valid.empty()) return valid;
        console.printLine("No valid stage given on the command line; asking interactively instead.");
    }
    return promptInteractive(console, available);
}
