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
