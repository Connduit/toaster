# rtlsdr_fm (aka "Toaster")

A minimal wideband-FM broadcast receiver for the RTL-SDR, written in C++,
built as an extensible `Pipeline` of independent `PipelineStage`s rather
than a single hardcoded audio path.

## Pipeline

```
RTL-SDR dongle
  -> uint8 IQ samples @ 1.92 MS/s (default; configurable via -r)
  -> Receiver converts to std::complex<float>, hands buffers to a callback
  -> Toaster's callback wraps each buffer as an IQMessage and dispatches it
  -> Dispatcher routes it to every stage subscribed to that message type
       -> AudioStage:          demod -> anti-alias filter -> decimate -> de-emphasis -> sink
       -> SpectrumStage:       window -> FFT -> dBFS -> frame averaging -> renderer
       -> ClassificationStage: window -> FFT -> dBFS -> noise floor -> run detection -> catalog match -> sink
       -> ScannerStage:        RSSI -> threshold -> dispatches "tune <hz>" (see below)
```

There are four stages today, independently selectable via `-s` (see
"Run" below). `AudioStage` produces 16-bit mono PCM, either piped to
stdout (for `aplay`/`sox`/`ffplay`) or written to a `.wav` file.
`SpectrumStage` produces a live on-screen view of the whole captured
band of spectrum (1.92 MHz by default, configurable via `-r`), drawn to
**stderr** so it can run at the same time as audio being piped out of
stdout. `ClassificationStage` measures signals in the captured band and
makes a feature-based guess at what they are, against a small built-in
table — see "Scanning and classification" below. `ScannerStage` is the
one stage that retunes the shared hardware: it sweeps a configured
frequency range looking for strong signals and locks onto one when
found, and composes with the other three stages with zero direct
coupling between any of them (also covered below).

The point of the `PipelineStage` split is that adding a feature means
writing one new stage and adding one line to `Toaster`. Adding
`ClassificationStage`/`ScannerStage` on top of `AudioStage`/
`SpectrumStage` is the second worked example: it required two entries
in `allStageDescriptors()`, one new command ("tune <hz>", handled by
`Controller`), and no change whatsoever to `Receiver`, `Dispatcher`, or
either existing stage's core logic (`AudioStage`/`SpectrumStage` each
gained a few lines to relabel/reset themselves when a retune happens,
since that's a real hardware event they need to react to — but neither
needed to know `ScannerStage` exists to do it). All four stages
subscribe to the same `IQMessage` (and, where relevant, `CommandMessage`)
independently; none is aware any of the others exist.

## Project structure

Files are organized by module — every module folder holds both its
headers and its `.cpp` files together, rather than splitting all headers
into one `include/` tree and all implementation into a separate `src/`
tree. That include/src split is really a library-packaging convention
(it exists to give a library a clean public API directory separate from
its private implementation); for a single application like this one,
it just means every "where's the header for X" question requires
checking two directories instead of one. Every `#include` is written
path-qualified relative to `src/` (e.g. `#include "dsp/filters/Filter.h"`),
so a dependency is unambiguous regardless of which file you're reading —
`target_include_directories` in `CMakeLists.txt` points at `src` itself.

```
src/
├── core/           Config, Toaster, Controller, PipelineStage, StageRegistry, StageSelection
├── messaging/      Dispatcher, Messages, InternalMessageType
├── rf/             Receiver, RtlSdrSource
├── dsp/            Fft, WindowFunction, DigitalDownConverter
│   └── filters/    Filter hierarchy (see below), Decimator
├── audio/          AudioStage, AudioSink, PcmWriter, WavFileWriter, FmDemodulator
├── spectrum/       SpectrumStage, SpectrumSink, SpectrumFrame, SpectrumView,
│                   SpectrumCommands, renderers
├── classification/ ClassificationStage, ClassificationSink, DeviceCatalog
├── scanner/        ScannerStage, ScannerSink
├── console/        Console, Listener, ConsoleCommandListener
└── main.cpp
```

| File | Class | Responsibility |
|---|---|---|
| `src/core/Config.h` | `ReceiverConfig`, `AudioStageConfig`, `ToasterConfig`, `ClassificationStageConfig`, `ScannerStageConfig`, `AudioOutputMode`, `FilterImplementation` | All configuration structs/enums, in one place. No logic — just settings |
| `src/core/Toaster.h` / `.cpp` | `Toaster` | The top-level object. Owns a `Receiver`, a `Dispatcher`, a `Controller`, a `Console`, and a `ConsoleCommandListener`, and wires them together — including building whichever `Filter`, `AudioSink`/`SpectrumSink`/`ClassificationSink`/`ScannerSink`, and `PipelineStage`s the config asks for. This is the only place in the program that picks concrete types |
| `src/core/Controller.h` / `.cpp` | `Controller` | Subscribes to `CommandMessage` and acts on the real subsystems: `"stop"` stops the `Receiver`, `"tune <hz>"` retunes it (see `ScannerStage` below) |
| `src/rf/Receiver.h` / `.cpp` | `Receiver` | Purely RF concerns: open the device, tune it, retune it live, set gain, stream raw IQ via a plain callback. Has no idea `Dispatcher` or `IQMessage` exist — see the class comment for why |
| `src/rf/RtlSdrSource.h` / `.cpp` | `RtlSdrSource` | Owns the librtlsdr device handle; the low-level C API wrapper `Receiver` sits on top of |
| `src/console/Console.h` | `Console` | Thin wrapper around stdin/stderr. Centralizes console I/O so nothing else touches `std::cin`/`std::cerr` directly |
| `src/console/Listener.h` | `Listener` | Abstract base for anything that watches for an event on a background thread and fires a callback |
| `src/console/ConsoleCommandListener.h` | `ConsoleCommandListener` | A `Listener` that reads lines typed at the console and dispatches each as a `CommandMessage` |
| `src/core/PipelineStage.h` | `PipelineStage` | Abstract interface: a self-contained consumer of the `Dispatcher`'s messages (`flush`/`reset`). Each stage subscribes itself to whatever message types it needs directly, from its own constructor — there's no central fan-out class calling a uniform `processBuffer` |
| `src/audio/AudioStage.h` / `.cpp` | `AudioStage` | A `PipelineStage`. Owns the audio DSP chain: demod → anti-alias filter → decimate → de-emphasis → sink. Depends only on the abstract `Filter` and `AudioSink` interfaces, not concrete types. Also listens for `"tune <hz>"` purely to reset its DSP state around a retune |
| `src/audio/FmDemodulator.h` | `FmDemodulator` | Quadrature FM demodulation (delta-phase method), header-only |
| `src/dsp/filters/DeemphasisFilter.h` | `DeemphasisFilter` | Single-pole IIR de-emphasis. Implements `Filter` for consistency, though it's a simpler shelf, not one of the LP/HP/BP/BS shapes |
| `src/dsp/filters/Decimator.h` | `Decimator` | Keeps 1 out of every N samples it's given, header-only |
| `src/audio/PcmWriter.h` | `PcmWriter` | `AudioSink` that writes raw 16-bit PCM to a `FILE*` (stdout by default) |
| `src/audio/WavFileWriter.h` / `.cpp` | `WavFileWriter` | `AudioSink` that writes a proper `.wav` file, patching the RIFF header sizes once the total sample count is known |
| `src/dsp/Fft.h` / `.cpp` | `Fft` | In-place radix-2 Cooley-Tukey FFT. Size fixed at construction so twiddle factors and the bit-reversal permutation are computed once |
| `src/dsp/WindowFunction.h` | — | Hann analysis window and its coherent gain, header-only |
| `src/dsp/DigitalDownConverter.h` / `.cpp` | `DigitalDownConverter` | Mixes a slice of the IQ stream to DC, filters and decimates it as a cascade of decimate-by-2 stages. What makes spectrum zoom improve resolution rather than just magnify bins |
| `src/spectrum/SpectrumView.h` | `SpectrumView` | Which slice of the captured band is on screen: centre offset and decimation |
| `src/spectrum/SpectrumCommands.h` / `.cpp` | — | Parses the `zoom` / `center` console commands, and `parseFrequency()`, a standalone frequency-string parser reused by `Controller` (for `"tune <hz>"`) and `main.cpp` (for `--scan-*` flags). Separate from the stage so it's testable without a dispatcher, receiver, or terminal |
| `src/spectrum/SpectrumStage.h` / `.cpp` | `SpectrumStage` | A `PipelineStage`. Owns the IQ-to-spectrum chain: window → FFT → dBFS → frame averaging → sink. Depends only on the abstract `SpectrumSink`. Also listens for `"tune <hz>"` to relabel itself and drop the stale average |
| `src/spectrum/SpectrumFrame.h` | `SpectrumFrame` | One finished measurement: dBFS per bin plus the axis information needed to draw it. Non-owning, like the types in `Messages.h` |
| `src/spectrum/SpectrumSink.h` | `SpectrumSink` | Abstract interface for whatever displays frames — the visual counterpart to `AudioSink` |
| `src/spectrum/TerminalSpectrumRenderer.h` / `.cpp` | `TerminalSpectrumRenderer` | Abstract `SpectrumSink`. Implements all the terminal handling once: sizing, resize, ANSI lifecycle, header readout, frequency axis, single buffered write per frame |
| `src/spectrum/BarSpectrumRenderer.h` / `.cpp` | `BarSpectrumRenderer` | Level-vs-frequency trace with a dB scale. Precise amplitude, no history |
| `src/spectrum/WaterfallRenderer.h` / `.cpp` | `WaterfallRenderer` | Frequency-vs-time, level as colour. Keeps history, coarse amplitude |
| `src/spectrum/TerminalCanvas.h` | — | Terminal size query, ANSI escapes, colour ramp, partial-block glyphs |
| `src/classification/DeviceCatalog.h` / `.cpp` | — | A small built-in table of known frequency allocations, and `matchDevice()`, a standalone feature-based matcher. No machine learning, no dependency on a dispatcher or a stage — testable on its own |
| `src/classification/ClassificationStage.h` / `.cpp` | `ClassificationStage` | A `PipelineStage`. Its own independent window → FFT → dBFS → noise-floor estimate → run detection chain (deliberately not sharing `SpectrumStage`'s, since that view can be zoomed by the user) → `DeviceCatalog::matchDevice()` → dispatches a `SignalDetectionMessage` and reports to a `ClassificationSink` |
| `src/classification/ClassificationSink.h` | `ClassificationSink` | Abstract interface for whatever records detections |
| `src/classification/TerminalClassificationSink.h` / `.cpp` | `TerminalClassificationSink` | Writes one line per detection to a file (default) or stderr |
| `src/scanner/ScannerStage.h` / `.cpp` | `ScannerStage` | A `PipelineStage`. Sweeps a frequency range measuring broadband RSSI (no FFT — see the class comment for why), and dispatches `"tune <hz>"` `CommandMessage`s to retune the shared hardware. The only stage that retunes; see "Scanning and classification" below |
| `src/scanner/ScannerSink.h` | `ScannerSink` | Abstract interface for whatever records lock/resume events |
| `src/scanner/TerminalScannerSink.h` / `.cpp` | `TerminalScannerSink` | Writes one line per lock/resume event to a file (default) or stderr |
| `src/main.cpp` | — | CLI argument parsing; builds a `Toaster` and runs it |

### Filter hierarchy

```
Filter                          (abstract: process(sample), reset())
├── FirFilter                   (abstract: circular-buffer convolution)
│   ├── FirLowPassFilter
│   ├── FirHighPassFilter
│   ├── FirBandPassFilter
│   └── FirBandStopFilter
├── IirFilter                   (abstract: direct-form-I biquad)
│   ├── IirLowPassFilter
│   ├── IirHighPassFilter
│   ├── IirBandPassFilter
│   └── IirBandStopFilter
└── DeemphasisFilter            (single-pole shelf, its own thing)
```

- `FirFilter` implements the convolution math once; each FIR subclass just designs coefficients (`FirDesign.h`/`.cpp` — windowed-sinc/Hamming, with high-pass and band-stop built via spectral inversion) and calls `setCoefficients()`.
- `IirFilter` implements the direct-form-I biquad difference equation once, using clearly-named state (`feedforward0_`/`feedforward1_`/`feedforward2_` for the b coefficients, `feedback1_`/`feedback2_` for the a coefficients, `previousInput1_`/`previousInput2_`/`previousOutput1_`/`previousOutput2_` for the delay line) rather than the terser `b0_`/`x1_`/`y1_` convention some DSP code uses. Each IIR subclass just designs coefficients (`IirDesign.h`/`.cpp` — standard "Audio EQ Cookbook" formulas) and calls `setCoefficients()`.
- **Not templates.** Every sample in this project is `float` — there's no second type these filters would ever need to work over, so genericity via templates would buy nothing. It would also cost something real: `Filter*`/`std::unique_ptr<Filter>` gives *runtime* polymorphism, which is what lets `Toaster` pick FIR vs. IIR based on a CLI flag (`-t fir|iir`) decided at runtime. Templates are resolved at compile time and can't do that — you'd need a non-template base class for dynamic dispatch anyway, which is exactly what `Filter` already is.
- `Filters.h` is a convenience header that pulls in all eight concrete classes plus the two abstract bases — include it wherever you need to pick a filter type at construction time (only `Toaster.cpp` does this; `AudioStage` only needs `Filter.h`).
- `AudioStage`'s anti-aliasing stage takes a `std::unique_ptr<Filter>` in its constructor rather than owning a concrete filter, the same dependency-injection pattern already used for `AudioSink`. `Toaster` decides FIR vs IIR and builds the right one; `AudioStage` never needs to change to support a new filter shape or implementation.
- The anti-alias stage must be low-pass for correctness — it exists to prevent aliasing before decimation, and swapping in a high-pass there would break demodulation. What's swappable is the *implementation* (FIR vs IIR), not the *shape*. The high-pass/band-pass/band-stop classes exist as general-purpose building blocks (e.g. a future squelch pre-filter or notch filter for a known interferer) rather than being wired into the anti-alias slot.
- IIR band-pass/band-stop are single biquads (2nd order), so they roll off much more gently than the FIR versions at the same edges — that's an inherent property of a single second-order section, not a bug.

### Why separate config structs instead of one `ToasterConfig` everywhere?

`Receiver` takes a `ReceiverConfig`, `AudioStage` takes an `AudioStageConfig`
— neither takes the full `ToasterConfig`, even though `Toaster` has one and
could just hand it to everybody. This is deliberate: `Receiver` has no
business knowing about `audioSampleRateHz`, and `AudioStage` has no
business knowing about `deviceIndex`. Giving each class only the config
it actually needs keeps its dependencies self-documenting (you can read
`AudioStageConfig` and know exactly what `AudioStage` uses, full stop),
avoids the temptation to reach for a field that "happens to be right
there" but doesn't belong to that class's job, and keeps `ToasterConfig`
from becoming a junk drawer as more stages (each with their own settings)
get added. `Toaster` is the one place allowed to know about everything,
since assembling everything is its job — it derives the narrower configs
from its own `ToasterConfig` via small conversion functions in
`Toaster.cpp` (`toReceiverConfig`, `toAudioStageConfig`).

### Why aren't `Toaster`'s members pointers?

`Toaster` holds `Receiver receiver_` and `Pipeline pipeline_` by value,
not as `Receiver*`/`Pipeline*`. Compare that to `AudioStage`, which *does*
hold pointers: `std::unique_ptr<Filter> antiAliasFilter_` and
`std::unique_ptr<AudioSink> sink_`. The difference is whether polymorphism
is needed. `Filter` and `AudioSink` are abstract interfaces — at runtime,
`antiAliasFilter_` might be a `FirLowPassFilter` or an `IirLowPassFilter`;
that dynamic dispatch requires a pointer/reference to the base type, full
stop. `Receiver` and `Pipeline`, by contrast, are concrete, single,
non-polymorphic types — there's only one kind of `Receiver`, and `Toaster`
owns exactly one, for exactly its own lifetime. A `Receiver*` member would
need manual `new`/`delete` (or a `unique_ptr`, at least safe) to get
precisely what value composition already gives for free: construction and
destruction tied to `Toaster`'s own constructor/destructor, no heap
allocation, no possibility of a null or dangling pointer, no extra
indirection on every access. The rule this project follows: value member
by default; pointer/`unique_ptr` only when polymorphism or optional/lazy
construction is actually needed.

Ownership tree:

```
Toaster                     (wires everything together, owns the object graph)
├── Receiver                (RF concerns only — nothing about audio/DSP)
│   └── RtlSdrSource
├── Console                 (wraps stdin/stderr)
├── ConsoleStopListener     (Listener; background thread watching Console for "stop")
└── stages_              (whichever stages were selected; see StageSelection)
    ├── AudioStage          (turns IQ into audio)
    │   ├── FmDemodulator
    │   ├── Filter*         (injected: FirLowPassFilter or IirLowPassFilter)
    │   ├── Decimator
    │   ├── DeemphasisFilter
    │   └── AudioSink*      (injected: PcmWriter or WavFileWriter)
    └── SpectrumStage       (turns IQ into a picture of the band)
        ├── DigitalDownConverter  (only engaged when zoomed)
        ├── Fft
        └── SpectrumSink*   (injected: BarSpectrumRenderer or WaterfallRenderer)
```

Note how symmetric the two stages are: each owns its DSP concretely and
takes its output destination as an injected abstract interface, so
neither knows where its results end up. `Toaster` is the only place that
picks `PcmWriter` vs `WavFileWriter`, or bars vs waterfall.

`Receiver` and `Pipeline` don't know about each other, and no
`PipelineStage` knows about any other stage. `Toaster::run()` is what
connects the Receiver to the Pipeline:

```cpp
receiver_.startAsync([this](const uint8_t* buf, uint32_t len) {
    pipeline_.processBuffer(buf, len);
});
```

and `Toaster`'s constructor is where stages get registered:

```cpp
pipeline_.addStage(std::make_unique<AudioStage>(
    toAudioStageConfig(config), makeAntiAliasFilter(config), makeSink(config)));

// A future stage plugs in exactly the same way, e.g.:
// pipeline_.addStage(std::make_unique<SpectrumAnalyzerStage>(...));
// pipeline_.addStage(std::make_unique<SignalClassifierStage>(...));
```

If you swapped `Receiver` for a file-based IQ source, or added a stage
that decodes NFM instead of WFM, none of the existing classes would need
to change — only `Toaster` would.

Note: filtering and decimation used to be combined into one class for
efficiency (only computing the FIR convolution on samples that survive
decimation, ~40x fewer multiplies). Splitting them trades a bit of that
efficiency for clearer separation of concerns — the filter now runs at
the full RF rate (1.92 MHz by default). On any modern CPU this is still
trivial (a few hundred
million multiply-adds/sec for a 63-tap filter), so it's a good trade here.


On Debian/Ubuntu:

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config librtlsdr-dev alsa-utils
```

You also want the RTL-SDR udev rules so you don't need root to access the
dongle, and you should blacklist the default `dvb_usb_rtl28xxu` kernel driver
that otherwise claims the device:

```bash
sudo apt install rtl-sdr
echo 'blacklist dvb_usb_rtl28xxu' | sudo tee /etc/modprobe.d/blacklist-rtl.conf
```

Unplug/replug the dongle (or reboot) after that.

If you're on WSL2, pass the USB device through with `usbipd` first
(`usbipd attach --wsl --busid <id>` from Windows), since WSL2 doesn't see USB
devices by default.

## Build

```bash
cd rtlsdr_fm
mkdir build && cd build
cmake ..
make
```

This produces `./rtlsdr_fm`.

## Run

Tune to a local FM station (frequency in Hz) and pipe to `aplay`:

```bash
./rtlsdr_fm -f 100100000 | aplay -r 48000 -f S16_LE -c 1
```

Or write straight to a `.wav` file instead:

```bash
./rtlsdr_fm -f 100100000 -o station.wav
```

While running, type `stop` and press Enter to end the program cleanly —
this flushes buffers and, in `.wav` mode, finalizes the file header.
Ctrl+C is intentionally not handled: cancelling an in-flight USB read from
a signal handler isn't safe, so `stop` is the supported way to end things.

Or watch the spectrum instead of listening to it:

```bash
./rtlsdr_fm -f 100100000 -s spectrum-visual
./rtlsdr_fm -f 100100000 -s spectrum-visual -v waterfall -n 2048
```

Or do both at once — the display goes to stderr, the audio to stdout:

```bash
./rtlsdr_fm -f 100100000 -s audio,spectrum-visual | aplay -r 48000 -f S16_LE -c 1
```

Or sweep a range for activity and identify whatever's found, while
listening to it — see "Scanning and classification" below for how this
works with zero coupling between the three stages involved:

```bash
./rtlsdr_fm -f 100100000 -s audio,scanner,classification \
    --scan-start 88M --scan-end 108M | aplay -r 48000 -f S16_LE -c 1
```

If you leave `-s` off entirely you'll be prompted for which stages to
run at startup.

Options:

- `-f <hz>` — center frequency in Hz (required), e.g. `100100000` for 100.1 MHz
- `-d <n>` — device index if you have more than one RTL-SDR (default 0)
- `-g <db|auto>` — tuner gain in dB (e.g. `-g 40`), or `auto` (default)
- `-o <file.wav>` — write a `.wav` file instead of streaming raw PCM to stdout
- `-t fir|iir` — anti-aliasing filter implementation (default: `fir`)
- `-s <stage,...>` — which stages to run: any combination of `audio`,
  `spectrum-visual`, `classification`, `scanner`
- `-v bars|waterfall` — spectrum display style (default: `bars`)
- `-n <bins>` — spectrum FFT size, a power of two (default: `1024`)
- `-r <hz>` — RF bandwidth captured (default: `1920000`, i.e. 1.92 MHz).
  This is also the widest span the spectrum display can ever "zoom out"
  to, since it only ever shows a slice of whatever was captured — see
  "Zooming and panning" below. Must be a rate librtlsdr's RTL2832U
  supports (`225001`-`300000` or `900001`-`3200000`); above roughly
  2.4-2.88 MS/s, USB throughput on some hosts can't keep up and samples
  get dropped, so test before relying on a rate that high. If you also
  run the audio stage, pick a multiple of 48000 (e.g. `2400000`,
  `2880000`) — anything else still works, but the audio stage's
  decimator truncates the ratio to an integer, so the actual output rate
  drifts slightly from 48 kHz (main.cpp's `-r` handling warns about this
  when it applies).
- `--scan-*` — scanner sweep range/timing/threshold, and where lock/resume
  events are logged. Only relevant when `scanner` is selected — see
  "Scanning and classification" below.
- `--classification-*` — detection sensitivity (FFT size, noise-floor
  margin, minimum run length) and where detections are logged. Only
  relevant when `classification` is selected — see below.

Run `./rtlsdr_fm` with no arguments (or an invalid flag) to see the full,
current list with defaults — `main.cpp`'s `printUsage()` is kept as the
single source of truth for exact flag names and defaults, rather than
duplicated here where it could drift out of sync.

Zoom and pan are driven by typing commands while it runs — see below.

## The spectrum display

`-v bars` draws the classic analyser trace: level against frequency,
with a dB scale down the left and the tuned frequency in the middle.
`-v waterfall` draws frequency across and time downward with level as
colour, which is the better view for spotting anything intermittent — a
burst, a drifting carrier, interference that comes and goes — since a
bar trace only ever shows you the current instant.

Both show the full captured band (1.92 MHz by default) the dongle is
receiving, not just the
station you're tuned to. Levels are dBFS: `0` means a full-scale tone
landing entirely in one bin, and the noise floor of a typical dongle
sits somewhere around `-70`.

### Zooming and panning

While it's running, type these and press Enter:

```
zoom in            halve the span (double the resolution)
zoom out           the reverse
zoom 200k          set the span directly — 200k, 1.5M, 7500 all work
zoom reset         back to the full captured span
center 99.7M       put an absolute frequency in the middle
center +200k       shift the view by a relative amount
center reset       back to the tuned frequency
help               show the above on the display
```

Feedback appears in the display's header for a few seconds rather than
being printed, since the display owns the terminal.

**Zooming does not retune the dongle.** The view only ever selects a
slice of whatever's already being captured (1.92 MHz by default,
wider with `-r`), so you can zoom and pan
the picture while the audio stage keeps playing the station it's tuned
to, undisturbed. The trade-off is that the view cannot leave the
captured band: `center 101.5M` while tuned to 100.1 MHz is out of reach,
and the command says so. Moving outside genuinely needs a hardware
retune, which would drag the audio along with it.

**Zoom is real, not magnification.** Cropping the FFT to its middle bins
would fill the screen with less span but show nothing new — the same
2.8 kHz bins, drawn wider. Instead the IQ is mixed down, filtered and
decimated first, so 1024 bins cover a proportionally narrower span and
each bin gets narrower with them. Watch the RBW readout drop as you zoom:
at 256x it reaches about 11 Hz, enough to separate two carriers 500 Hz
apart that are a single blob at full span.

The cost is time. One FFT window needs `zoom` times as many input
samples to fill, so deep zoom updates slowly — at 256x a single frame
needs about a fifth of a second of signal. That isn't an implementation
shortcut; resolving finer detail in frequency requires observing for
longer, and real analysers behave the same way. `maxDecimation` in
`Config.h` caps it at 256 for that reason.

A few further design points worth knowing:

- **Most windows are deliberately discarded.** At the default 1.92 MS/s the receiver
  produces about 1875 windows of 1024 samples every second. The stage
  transforms roughly twenty of them and throws the rest away, because a
  display refreshing faster than about 30 Hz tells a human nothing more,
  and this work happens on the RTL-SDR read thread — where overrunning
  the time budget means dropped USB transfers and, with the audio stage
  also running, audible glitches. The consequence is that the display
  cannot see events shorter than the refresh interval. That's fine for
  looking at a band and wrong for anything that must not miss a burst;
  such a stage would want every window, and would want it off the RF
  thread.
- **The Hann window isn't optional.** An FFT assumes its block of
  samples repeats forever, and a real signal doesn't line up with the
  block's ends, so that implied repetition has a discontinuity — which
  is broadband, and smears a strong local station across the entire
  display. Tapering to zero at both ends removes it. This is the
  difference between a usable noise floor and one 40 dB too high.
- **Frames are averaged in linear power, not in dB.** Averaging
  logarithms is a geometric mean: it biases low, and biases further the
  noisier the bin, so it would make the noise floor read lower than it
  actually is. Tune the amount with `smoothingFactor` in `Config.h`.
- **Columns take the maximum, not the mean, of the bins they cover.**
  With 1024 bins across 70-odd terminal columns, a narrow carrier
  occupies one bin; averaging it with its fifteen empty neighbours
  would drag it back down into the noise and hide exactly what you're
  looking for.
- **Decimation is a cascade of by-2 stages**, which is why zoom factors
  are powers of two. One filter decimating by 64 in a single step would
  need a transition band 64x narrower — hundreds of times more taps, all
  running at the full input rate. In a cascade every stage faces the
  same relative problem and each runs at half the rate of the one
  before, so the total work converges to about twice the first stage's
  however deep it goes. Each stage also computes only the samples it
  keeps, which is why this doesn't reuse `FirLowPassFilter` +
  `Decimator`: `Filter::process()` must return an output per input,
  which is exactly the work a decimating filter exists to avoid.
- **Nothing is demodulated.** `AudioStage` starts with `FmDemodulator`
  because it wants what's encoded on one carrier; `SpectrumStage`
  works on raw IQ, because "what's present across this band" is a
  property of the RF itself and demodulating would destroy it.

The display redraws in place and restores your cursor on exit. Resizing
the terminal works — the layout is re-measured every frame rather than
through a `SIGWINCH` handler, for the same async-signal-safety reasons
this program doesn't handle Ctrl+C. Redirecting stderr to a file drops
the colour and cursor control and falls back to ASCII, so
`2> capture.txt` gives you something readable rather than a pile of
escape sequences.

## Scanning and classification

Two more independently-selectable stages, `scanner` and `classification`,
compose with `audio` (and each other) with **zero direct coupling**
between any of them — none of the three has a pointer, reference, or
`#include` pointing at either of the others. All that ties them together
is the shared `Dispatcher`:

- `ScannerStage` sweeps `--scan-start` to `--scan-end` in `--scan-step`
  increments, classic-scanner-radio style: retune, measure broadband
  RSSI for `--scan-measure-ms` (after discarding `--scan-settle-ms` for
  the tuner's PLL to settle), and either move on or lock. A lock happens
  when RSSI crosses `--scan-threshold` (dBFS — this depends heavily on
  `-g`'s gain setting, so it's usually the first thing to retune if
  nothing ever locks, or everything does). Locked, it holds still for
  `--scan-lock-ms` before resuming. It retunes the *shared* hardware by
  dispatching `"tune <hz>"` as a `CommandMessage` — the same mechanism a
  human-typed console command would use — which is the only thing that
  makes the rest of this work.
- `AudioStage` was never told a scanner exists. It just demodulates
  whatever IQ arrives, so the moment the hardware retunes, it's
  demodulating the new frequency — "listening in" is a side effect of
  processing every buffer, not a feature anyone had to build.
- `ClassificationStage` likewise was never told a scanner exists. Its own
  independent FFT window naturally starts measuring the new frequency's
  spectrum, and once enough of a window has elapsed it reports whatever
  it finds there.

Run all three and "sweep, lock, listen, and identify" emerges from three
stages that have never heard of each other:

```bash
./rtlsdr_fm -f 100100000 -s audio,scanner,classification \
    --scan-start 88M --scan-end 108M --scan-threshold -25 \
    | aplay -r 48000 -f S16_LE -c 1
```

`ClassificationStage` is deliberately *not* built on `SpectrumStage`'s
FFT frames, even though the two computations look almost identical: a
user can zoom `SpectrumStage`'s display into a narrow slice at any time
(see "Zooming and panning" above), and classification needs to keep
seeing the *whole* captured band regardless of what's currently on
screen. So it runs its own independent window → FFT → dBFS chain,
estimates the noise floor as the ~20th-percentile bin value (robust
against a strong signal dragging a mean-based estimate upward), finds
runs of bins that cross `--classification-margin-db` above that floor
for at least `--classification-min-run-bins` bins, and matches each
run's power-weighted center frequency and width against a small
built-in table in `src/classification/DeviceCatalog.h` — FM broadcast,
NOAA weather radio, aviation/marine/amateur VHF, common ISM bands
(433/868/915 MHz), DECT, Wi-Fi/Bluetooth/Zigbee at 2.4 GHz, and so on.

**There is no machine learning here, on purpose.** Matching is a lookup
against known frequency/bandwidth ranges, not a learned model — an
honest "here's what this could plausibly be, given only where it sits
and how wide it is" rather than false confidence from a two-number
measurement. `DeviceCatalog::matchDevice()` is a free function with no
dependency on a dispatcher, a stage, or a receiver, specifically so it's
easy to test on its own and easy to extend with more entries later — see
its header comment. Real classification (modulation recognition, RF
fingerprinting, an actual model) would be a much larger, separate
project; this gives today's users a reasonable feature-based guess, not
a placeholder pretending to be something it isn't yet.

Every detection is both handed to a `ClassificationSink` directly *and*
dispatched as a `SignalDetectionMessage` on the `Dispatcher` (same for
`ScannerStage`'s lock/resume events and `ScanEventMessage`) — the direct
call is how its own log gets written, and the dispatch is what a future
stage (an alerting stage, say, or one that persists detections to a
database) could subscribe to without `ClassificationStage`/`ScannerStage`
ever needing to know it exists, the same way nothing producing an
`IQMessage` needs to know `AudioStage` exists.

**A real terminal conflict to know about:** `classification`/`scanner`
default to writing their logs to files (`classification.log`/`scan.log`)
rather than stderr, and this isn't just a style choice. `SpectrumStage`'s
renderer redraws in place using ANSI cursor movement on the assumption
that it owns everything below where it last drew (see
`spectrum/TerminalSpectrumRenderer.h`); any other writer appending plain
lines to that same stream — `classification`/`scanner`'s logs included,
if pointed at stderr with `--classification-log -` / `--scan-log -` —
corrupts that redraw the next time it runs, regardless of which thread
wrote what, purely because the cursor math no longer matches what's
actually on screen. `Toaster` warns once at startup if `spectrum-visual`
is combined with either log pointed at stderr; the fix is either point
the log at a file (the default) or drop `spectrum-visual` for that run.

## Tuning tips

- If the audio sounds distorted/clipped, your signal is probably too strong —
  try `-g` with a lower manual gain instead of `auto`.
- If it's noisy/weak, try a stronger gain (`-g 49.6` is close to max on most
  R820T2 dongles) or improve your antenna.
- `rtl_test` (from the `rtl-sdr` package) is useful for confirming the
  dongle itself works before debugging this program.
- The spectrum display is the quickest way to find stations: run
  `-s spectrum-visual`, tune anywhere in the FM band, and the peaks you
  see are broadcasts. The header names the strongest one's frequency,
  which you can then pass straight to `-f`.
- If the display looks blank, everything is below the bottom of the
  scale — raise gain with `-g`, or lower `spectrumMinDb` in `Config.h`.
  If it's saturated across the whole width, gain is too high.
- Broadcast FM stereo (the 38 kHz subcarrier / pilot tone) isn't decoded
  here — this recovers mono audio only. Adding stereo decoding (an FIR
  bandpass around 23–53 kHz plus a Costas loop to recover the pilot) is a
  natural next step if you want to extend this.

## Changing region / de-emphasis

The US and South Korea use 75us de-emphasis; nearly everywhere else uses
50us. If audio sounds a bit muffled or overly bright, change the default
`deemphasisMicroseconds` in `ToasterConfig` (`include/Config.h`) between
`75.0` and `50.0`, or wire up a `-e` CLI flag in `main.cpp` if you want it
switchable at runtime.

## Architecture notes

- **Sample rate**: 1.92 MS/s was chosen as the *default* because it
  divides evenly by 40 to give exactly 48 kHz audio, and it's a natively
  supported RTL-SDR rate. It's configurable via `-r`, which is also the
  only way to widen the spectrum display's maximum "zoom out" span,
  since that display never retunes the dongle (see "Zooming and
  panning" above) — it can only ever show what was actually captured.
  Picking a rate that isn't a multiple of 48000 still works, but the
  audio stage's output will drift slightly off 48 kHz; `-r`'s own
  validation explains this if it applies to the rate you pick.
- **FM demod**: uses the "delta phase" trick
  (`atan2(Im(x[n]*conj(x[n-1])), Re(x[n]*conj(x[n-1])))`) rather than
  differentiating `atan2` directly — this avoids phase-wrap glitches.
- **Filtering vs. decimation**: kept as two separate classes (`Filter`
  and `Decimator`) rather than one combined class, so each has a single
  responsibility. The anti-alias filter runs at the full 1.92 MHz rate;
  `Decimator` then keeps 1 out of every 40 filtered samples. This costs
  more CPU than a fused filter-then-decimate (which only computes output
  samples that survive decimation), but at these sample rates it's a
  non-issue on any modern machine.
- **Stopping**: a typed `stop` command on a background thread
  (`ConsoleCommandListener`, dispatched as a `CommandMessage` like any
  other console command) rather than Ctrl+C/SIGINT, since calling
  `rtlsdr_cancel_async()` (or `fprintf`, or most anything else) from a
  real signal handler isn't guaranteed safe mid-USB-transfer or
  mid-stdio-call. A normal thread has no such restriction.
- **`Receiver` doesn't know `Dispatcher` exists**: it takes a plain
  `std::function<void(const std::complex<float>*, size_t)>` callback
  instead, and `Toaster` (the one place responsible for wiring
  components together) supplies a callback that wraps each buffer as an
  `IQMessage` and dispatches it. This costs nothing at runtime — it's
  still one synchronous call on the same RF thread, just with one layer
  of indirection — and in exchange `Receiver` stays testable/reusable
  without dragging in the whole messaging system.
- **One command bus, not one message type per command**: `"stop"`,
  `"zoom in"`, `"tune 100300000"` are all just a `CommandMessage`'s
  `name` string, and every interested party (`Controller`,
  `SpectrumStage`, `AudioStage`, `ClassificationStage`) subscribes to
  the same broadcast and parses out whatever it recognizes. This is
  what lets `ScannerStage` retune the hardware, and every other
  frequency-aware stage relabel/reset itself around that retune,
  without any of them holding a reference to each other or to
  `Controller`/`Receiver`.
- **`ScannerStage` measures RSSI, not an FFT**: `SpectrumStage` and
  `ClassificationStage` both need to know *where within* a captured
  band a signal sits, so they transform. `ScannerStage` only ever asks
  "is there something strong at the exact frequency I'm now tuned to",
  which total signal power answers directly — cheaper, and one less
  thing coupling it to `dsp/Fft.h`/`dsp/WindowFunction.h`.
