#pragma once

#include <string>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/ioctl.h>
#include <unistd.h>
#endif

// Low-level terminal drawing helpers shared by the spectrum renderers:
// how big the window is, the escape sequences for moving/colouring, and
// the glyphs used to draw a bar with sub-character precision.
//
// Kept here rather than duplicated into BarSpectrumRenderer and
// WaterfallRenderer (which would otherwise both grow their own copy of
// the same colour ramp and the same "what size is the terminal"
// ioctl), and kept out of console/Console.h because Console is
// deliberately a *line-oriented* stdin/stderr wrapper -- teaching it
// about cursor addressing and 256-colour codes would widen its job
// considerably for the benefit of one stage.
namespace terminal {

struct Size {
    int columns;
    int rows;
};

// Current size of the terminal attached to `fd`. Falls back to a
// conventional 80x24 when there's no terminal (output redirected to a
// file or a pipe) or the query fails. Cheap enough to call once per
// frame, which is what makes live resizing work without a SIGWINCH
// handler: the next frame simply measures again.
inline Size querySize(int fd, int fallbackColumns = 80, int fallbackRows = 24) {
#if defined(TIOCGWINSZ)
    struct winsize ws {};
    if (ioctl(fd, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        return Size{static_cast<int>(ws.ws_col), static_cast<int>(ws.ws_row)};
    }
#else
    (void)fd;
#endif
    return Size{fallbackColumns, fallbackRows};
}

// True if `fd` is an interactive terminal. When it isn't, the renderers
// drop the cursor-addressing and colour escapes: writing those into a
// log file or a pipe produces unreadable garbage rather than a display.
inline bool isInteractive(int fd) {
#if defined(__unix__) || defined(__APPLE__)
    return isatty(fd) == 1;
#else
    (void)fd;
    return false;
#endif
}

// ANSI/VT100 control sequences. Named functions rather than raw escapes
// scattered through the drawing code, because "\x1b[2J" at a call site
// tells the next reader nothing.
inline const char* cursorHome() { return "\x1b[H"; }
inline const char* clearScreen() { return "\x1b[2J"; }
inline const char* eraseToLineEnd() { return "\x1b[K"; }
inline const char* hideCursor() { return "\x1b[?25l"; }
inline const char* showCursor() { return "\x1b[?25h"; }
inline const char* resetStyle() { return "\x1b[0m"; }

inline std::string foreground(int color256) {
    return "\x1b[38;5;" + std::to_string(color256) + "m";
}

inline std::string background(int color256) {
    return "\x1b[48;5;" + std::to_string(color256) + "m";
}

// A cold-to-hot ramp through the xterm-256 cube: deep blue for the
// noise floor, through cyan and green, to yellow and red for the
// strongest signals. This is the convention essentially every SDR
// spectrum tool uses, so the display reads the way someone coming from
// SDR#/GQRX/rtl_power expects it to.
inline int heatColor(float normalized) {
    static const int kRamp[] = {17,  18,  19,  20,  26,  32,  38,  44,
                                 49,  48,  46,  118, 154, 184, 214, 196};
    constexpr int kRampSize = static_cast<int>(sizeof(kRamp) / sizeof(kRamp[0]));

    if (!(normalized > 0.0f)) return kRamp[0];  // also catches NaN
    if (normalized >= 1.0f) return kRamp[kRampSize - 1];
    return kRamp[static_cast<int>(normalized * kRampSize)];
}

// Partial block glyphs, U+2581 through U+2588, indexed by how many
// eighths of a cell are filled. Index 0 is a space. These let a bar end
// partway through a character cell, which multiplies the effective
// vertical resolution of the display by eight for free -- a 20-row
// plot resolves 160 levels rather than 20, which matters a great deal
// when the whole scale is only 90 dB tall.
inline const char* eighthBlock(int eighths) {
    static const char* kBlocks[] = {" ", "\u2581", "\u2582", "\u2583", "\u2584",
                                     "\u2585", "\u2586", "\u2587", "\u2588"};
    if (eighths < 0) eighths = 0;
    if (eighths > 8) eighths = 8;
    return kBlocks[eighths];
}

}  // namespace terminal
