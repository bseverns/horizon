# Horizon JUCE plugin playground

This `plugins/` corner is a host-facing, JUCE-flavored wrapper around the same
Horizon DSP blocks we run on Teensy. Think of it as a studio notebook page:
ready for experiments, but annotated so future you (or your students) can trace
what happens between a knob twist and a stereo mix.

## Build quickstart

```bash
cmake -S plugins -B plugins/build
cmake --build plugins/build
```

The CMake target fetches JUCE 7 at configure time and builds both a VST3 and a
standalone app. If your system already ships JUCE, feel free to swap the
`FetchContent` block for your local install — this file stays intentionally
minimal so you can riff on it.

> VST2 SDK missing? Same. The CMake target explicitly flips
> `JUCE_VST3_CAN_REPLACE_VST2` off so JUCE won't go hunting for Steinberg's
> retired headers. You still get a clean VST3 + standalone build without
> spelunking old SDK archives.

### CI guardrails (Mac, Windows, Linux)

GitHub Actions now runs the same configure + build steps on macOS, Windows, and
Linux. It’s a portable smoke test: if the bot can spit out a VST3 and a
standalone app on all three OSes, you can probably keep riffing without hunting
for missing system libs. Linux runners install the usual X11/ALSA deps; macOS
and Windows lean on their stock toolchains.

### Quick note on CMake warnings

CMake 3.28+ ships policy **CMP0175**, which complains about
`add_custom_command(TARGET ... DEPENDS)`. JUCE still uses that pattern inside
its own helpers, so this project pins the policy to `OLD` to keep the configure
log from filling up with noise while we focus on the DSP experiments.

## What lives here

- **`Source/`**: A JUCE `AudioProcessor` and editor that forward every control to
  `HostHorizonProcessor` (the same host wrapper used in tests).
- **`stubs/`**: Tiny Arduino/Audio header shims so the Teensy-centric limiter can
  compile in a desktop build without dragging in the full toolchain.

Feel free to extend the UI or add your own meter doodles. Just keep the intent
visible: Horizon is a teaching tool first, a sonic scalpel second.

## VST + standalone control map (what every slider actually pokes)

The editor uses JUCE sliders everywhere, so both the VST3 and the standalone
app expose the exact same controls. Hover a knob to see which DSP setter you’re
tickling; here’s the cheat sheet so you don’t have to guess:

- **Width** → `setWidth()` – static stereo spread. 0 = mono, 1 = max width.
- **Dyn Width** → `setDynWidth()` – how hard transients tug the sides inward
  before the tail reopens.
- **Transient** → `setTransientSens()` – detector sensitivity that feeds width
  motion and limiter release tweaks.
- **Tilt** → `setMidTilt()` – mid channel tilt EQ, ±6 dB/oct around ~1 kHz.
- **Air Freq** → `setSideAir(freq)` – pivot for the side shelf/bell (Hz).
- **Air Gain** → `setSideAir(gain)` – boost/cut for that side shelf (dB).
- **Low Anchor** → `setLowAnchor()` – mono fold point for bass (Hz).
- **Dirt** → `setDirt()` – gentle pre-limiter soft clip drive.
- **Ceiling** → `setCeiling()` – limiter output ceiling (dBFS). This and the
  mix/tilt controls let you steer how surgical or squishy the limiter feels.
- **Release** → `setLimiterReleaseMs()` – base release time (ms); transients
  can force it faster.
- **Lookahead** → `setLimiterLookaheadMs()` – detector lead time (ms) so the
  clamp lands before audio escapes.
- **Detector Tilt** → `setLimiterDetectorTilt()` – HF bias for the limiter
  detector (dB/oct) to tame splashy cymbals.
- **Limit Mix** → `setLimiterMix()` – limiter wet/dry. 0 = dry, 1 = all clamp.
- **Mix** → `setMix()` – global wet/dry for the whole Horizon chain.
- **Out Trim** → `setOutputTrim()` – post-limiter trim to nail the meter.
- **Limit Link (combo box)** → `setLimiterLinkMode()` – choose Linked or Mid/Side
  detection. Handy when width and limiter feel like they’re fighting.

Width and limiter are the headline moves here: start with Width around 0.6 for a
stable stereo bed, then ride Dyn Width and Transient until hits tuck in. On the
limiter, shape the feel with Ceiling + Release + Lookahead; Detector Tilt keeps
the HF clamp musical, and Limit Mix gives you a quick “is this vibe worth it?”
test without re-patching.
