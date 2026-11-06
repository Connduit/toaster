#pragma once

#include <functional>
#include <memory>
#include <string>

#include "core/Config.h"
#include "core/PipelineStage.h"
#include "messaging/Dispatcher.h"

// Describes one selectable pipeline stage: a short key the user types on
// the console (or passes to -s), a human-readable blurb shown in the
// stage-selection prompt, and a factory that builds the stage once it's
// been selected.
//
// Toaster builds a list of these (see allStageDescriptors() in
// Toaster.cpp) and loops over it rather than hard-coding a
// stages_.push_back() call per stage. Adding a new stage -- a spectrum
// visualizer, an electronic-spectrum analyzer, a signal classifier --
// means adding one entry to that list. It automatically shows up in the
// selection prompt and becomes independently optional: nothing forces it
// to run alongside any other stage, which matters once some stages are
// expensive (a spectrum display) and a user may only want one running at
// a time.
struct StageDescriptor {
    std::string key;          // what the user types, e.g. "audio"
    std::string description;  // shown next to the key in the prompt
    std::function<std::unique_ptr<PipelineStage>(const ToasterConfig&, Dispatcher&)> factory;
};
