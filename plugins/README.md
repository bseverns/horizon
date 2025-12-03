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

## What lives here

- **`Source/`**: A JUCE `AudioProcessor` and editor that forward every control to
  `HostHorizonProcessor` (the same host wrapper used in tests).
- **`stubs/`**: Tiny Arduino/Audio header shims so the Teensy-centric limiter can
  compile in a desktop build without dragging in the full toolchain.

Feel free to extend the UI or add your own meter doodles. Just keep the intent
visible: Horizon is a teaching tool first, a sonic scalpel second.
