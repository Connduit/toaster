#pragma once

// Convenience header: pulls in every concrete filter shape/implementation
// in the project. Include this where you need to pick a filter type at
// construction time (e.g. Toaster.cpp); AudioPipeline itself only needs
// the abstract Filter.h interface.

#include "dsp/filters/Filter.h"
#include "dsp/filters/AdjustableLowPassFilter.h"
#include "dsp/filters/FirFilter.h"
#include "dsp/filters/IirFilter.h"

#include "dsp/filters/FirBandPassFilter.h"
#include "dsp/filters/FirBandStopFilter.h"
#include "dsp/filters/FirHighPassFilter.h"
#include "dsp/filters/FirLowPassFilter.h"

#include "dsp/filters/IirBandPassFilter.h"
#include "dsp/filters/IirBandStopFilter.h"
#include "dsp/filters/IirHighPassFilter.h"
#include "dsp/filters/IirLowPassFilter.h"
