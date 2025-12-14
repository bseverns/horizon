# Host-side I/O adapters (CLI, plugins, live demos)

Horizon is happiest on Teensy, but the host-friendly DSP build means you can
bounce mixes and jam in real time on macOS, Windows, or Linux. This page is part
workbench, part studio notebook: every adapter shows how the same mid/side,
width, and limiter brain slots into different host contexts.

## CLI: `horizon-cli`

A headless renderer built on libsndfile and `HostHorizonProcessor`. It eats
WAV/AIFF/FLAC, applies a preset, and spits out a new file—perfect for CI,
A/B tests, or printing graphs.

```bash
cmake --build cmake-out/linux-clang --target horizon_cli
cmake --build cmake-out/linux-clang --target sync_compile_commands  # keep clangd happy

./cmake-out/linux-clang/horizon_cli input.wav output.flac --preset bus_glue
./cmake-out/linux-clang/horizon_cli input.aiff output.wav --preset width_extremes --block 256
./cmake-out/linux-clang/horizon_cli --list-presets
```

Presets mirror the JSON files in `presets/` (bus_glue, crunch_room,
width_extremes). If libsndfile is missing, CMake will skip the target and tell
you to install it (`libsndfile-dev` on Debian-ish machines).

Want the Teensy serial scope vibe on the host? Pass `--scope` to log the live
dynamic width pulse, transient meter, and limiter gain reduction every few
blocks while the render runs.

## Plugins: JUCE wrapper

`plugins/` already builds a VST3 and a standalone app via JUCE 7. It leans on
`HostHorizonProcessor` directly, so the same parameters and smoothing you see on
Teensy show up in the DAW. Quick build:

```bash
cmake -S plugins -B plugins/build
cmake --build plugins/build
```

CLAP/AUv3 formats are a small edit away: change the `FORMATS` line in
`plugins/CMakeLists.txt` to match your host. The UI is intentionally spartan so
students can trace knob → setter → sound without fighting a widget jungle.

## Real-time host demo: PortAudio

`horizon_portaudio` is a barebones live loop: default input to default output,
Horizon sitting in the middle with conservative settings. It is intentionally
small so you can swap in your own parameter maps or feed it MIDI later.

```bash
cmake --build cmake-out/linux-clang --target horizon_portaudio
./cmake-out/linux-clang/horizon_portaudio
```

PortAudio is optional; if the headers/libs aren’t present, CMake will skip the
target and print a reminder (`portaudio-dev` on Debian-ish boxes). The demo
keeps allocations out of the callback and fires a console nudge whenever the
limiter clip guard trips, keeping the real-time hygiene intact.

## Why these three?

- **CLI** keeps CI honest and lets you hand students a reproducible “before/after” sample.
- **Plugin** shows how to map the embedded pipeline into DAW land without hiding the DSP.
- **PortAudio loop** is the quickest way to hear parameter tweaks while projecting code in a classroom.
