# Horizon DSP — Host-side test bench

Think of this as the "headless rehearsal room" for Horizon. SeedBox-style, but
leaner: a PlatformIO `native` target that runs the DSP core on your laptop so
you can poke it without a Teensy on the desk.

## What this is
- Tiny Arduino/Audio stubs in `stubs/` that pin `AUDIO_SAMPLE_RATE_EXACT` and
  `AUDIO_BLOCK_SAMPLES` to the familiar 44.1 kHz / 128-sample world, plus an
  `AudioStream` shim that feeds horizon blocks without polluting the STL with
  Arduino-style macros.
- Unity-based smoke tests that push the limiter, smoother, tilt/air/dirt blocks
  and make sure nothing explodes when fed real signal ranges. The host-processor
  test even synthesizes its own mid/side-rich buffer so you don't need to haul
  any WAV fixtures around.
- A repeatable CI-friendly way to say "the math still holds" before you flash
  hardware.

## Run it
```sh
pio test -e native_dsp
```
The env keeps firmware-only pieces (wiring, AudioStream plumbing) out of the
build so you only compile the pure DSP bits. `platformio.ini` pins
`test_dir = test/native_dsp` and `test_build_src = yes`, so the runner
hoovers up this folder directly and still links the Horizon core even when
firmware entry points are filtered out. It stands alone—no Arduino or board
inheritance—so PlatformIO never nags you for hardware hints. CI runs this
target on every push, right next to full Teensy builds, so regressions have
nowhere to hide.

## Why bother?
- Fast iteration: catch logic regressions without hunting for a spare Teensy.
- Education: students can trace the DSP math in a debugger that isn't juggling
  USB audio.
- Confidence: when you refactor a saturator or tilt EQ, a native test can prove
  you didn't break the groove.

