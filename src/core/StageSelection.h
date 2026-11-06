#pragma once

#include <string>
#include <vector>

#include "console/Console.h"
#include "core/StageRegistry.h"

// Turns "what the user asked for" into "which stage keys to actually
// build", against the given list of available stages:
//
//   - If `requested` is non-empty (the user passed -s on the command
//     line), each key is validated against `available`. Unknown keys are
//     dropped with a warning printed via `console`. If that leaves at
//     least one valid key, those are returned with no prompting -- this
//     is what lets -s be used non-interactively (scripts, piping stdin
//     from /dev/null, etc.).
//   - Otherwise (or if every requested key turned out to be invalid),
//     the available stages and their descriptions are printed via
//     `console`, and the user is prompted to type a comma-separated list
//     of keys. Blank input selects the first available stage as the
//     default. The prompt repeats until at least one valid key is
//     entered, or the input stream closes (EOF), in which case the
//     first available stage is used as the default rather than hanging
//     forever.
//
// Always returns at least one key, as long as `available` is non-empty.
std::vector<std::string> resolveStages(Console& console, const std::vector<StageDescriptor>& available,
                                        const std::vector<std::string>& requested);
