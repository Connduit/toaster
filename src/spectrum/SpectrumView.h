#pragma once

// Which slice of the captured band the spectrum display is currently
// showing, expressed relative to whatever the dongle is tuned to.
//
// Both fields describe a *view*, not the hardware. The RTL-SDR stays
// tuned where it is; changing these only changes what the spectrum
// stage does with the samples it's already receiving. That's the whole
// point: zooming and panning the display must not retune the dongle,
// because the audio stage may be listening to a station through that
// same tuner, and having the picture move the sound would be a nasty
// surprise.
//
// The consequence is that the view is confined to the captured band --
// you can look anywhere within the rfSampleRateHz the dongle is
// receiving (1.92 MHz by default, configurable via -r), but not outside
// it. Going further afield genuinely requires either retuning the
// hardware (a different operation with different consequences) or
// capturing more bandwidth in the first place (raising rfSampleRateHz).
struct SpectrumView {
    // Centre of the view, as an offset in Hz from the tuned frequency.
    // Zero means the middle of the captured band.
    //
    // This is what the user *asked for*, deliberately not clamped to
    // what's currently displayable. How far the centre can move depends
    // on the span -- at full span it can't move at all, since the view
    // already covers everything captured -- so clamping on the way in
    // would make commands order-dependent: "center +300k" then "zoom
    // 30k" would lose the centre, while the same two commands in the
    // other order would work. Storing the request and clamping at the
    // point of use makes the two orders equivalent, which is what
    // someone typing them expects.
    double offsetHz = 0.0;

    // How much the IQ stream is decimated before the FFT sees it. 1 is
    // the full captured span; 2 shows half of it at twice the
    // resolution, and so on. Always a power of two (see
    // DigitalDownConverter).
    int decimation = 1;

    double spanHz(double rfSampleRateHz) const { return rfSampleRateHz / decimation; }

    // The requested offset, pulled back to wherever the view can
    // actually sit at the current span. Everything that displays or
    // measures uses this; only the command layer touches offsetHz.
    double effectiveOffsetHz(double rfSampleRateHz) const {
        const double span = spanHz(rfSampleRateHz);
        const double limit = (rfSampleRateHz - span) / 2.0;
        if (limit <= 0.0) return 0.0;
        if (offsetHz > limit) return limit;
        if (offsetHz < -limit) return -limit;
        return offsetHz;
    }

    double centerHz(double rfCenterHz, double rfSampleRateHz) const {
        return rfCenterHz + effectiveOffsetHz(rfSampleRateHz);
    }
};
