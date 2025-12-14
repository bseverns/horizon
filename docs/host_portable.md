# Host-friendly Horizon DSP library (Mac/Win/Linux)

This is the portable slice of Horizon: the mid/side matrix, tone shapers, dynamic width brain, limiter, and host-safe wrapper bundled into a CMake project. No Arduino headers, no Teensy specifics—just straight C++ you can drop into a DAW wrapper, command-line renderer, or live-coding sandbox.

## TL;DR build

- Presets for every desktop target live in `CMakePresets.json` so you can one-line the configure/build dance:

  ```bash
  cmake --preset linux-clang
  cmake --build --preset linux-clang
  # optional install
  cmake --install cmake-out/linux-clang --prefix /where/you/want/it
  ```

  - macOS Universal 2: `cmake --preset macos-universal-release` → `cmake --build --preset macos-universal-release`
  - Windows 11 + MSVC: `cmake --preset windows-msvc-release` → `cmake --build --preset windows-msvc-release`

- Want to drive CMake manually? The old flow still works:

  ```bash
  cmake -S . -B cmake-build -DCMAKE_BUILD_TYPE=Release
  cmake --build cmake-build --target horizon_dsp
  cmake --install cmake-build --prefix /where/you/want/it
  ```

- `sync_compile_commands` ships as a default target so the build mirrors the PlatformIO lint shim vibe: `cmake --build --preset linux-clang --target sync_compile_commands` keeps a fresh `compile_commands.json` at the repo root for clangd/IntelliSense.

The library ships with proper install rules and a `horizon_dsp::horizon_dsp` target, so downstream projects can simply `find_package(horizon_dsp CONFIG REQUIRED)` and link the alias.

Want to actually push air? Jump to [`host_io_adapters.md`](host_io_adapters.md) for the trio of adapters that hang off this
library: a libsndfile-based CLI, the JUCE plugin playground, and a minimal
PortAudio live loop.

## What you get
- Core DSP blocks: MS matrix, tilt and air EQs, dynamic width with low anchor, transient detector, soft saturation, and the limiter.
- `HostHorizonProcessor`: a lightweight glue class that mirrors the Teensy `AudioHorizon` signal path but speaks plain float buffers. Great for DAW bridges (JUCE/iPlug2), CLI renderers, or CI golden-file tests.
- Headers land under `include/horizon/` when installed to keep namespacing predictable.

## Design notes (why bother)
- This keeps the **real-time discipline** of the firmware builds: no allocations or I/O inside the per-sample loop, parameters are smoothed, and defaults are musically safe.
- It matches the teaching vibe: short classes with explicit units, comments that focus on intent, and APIs named after mix moves (width, dirt, air) instead of opaque math.
- The CMake layout is intentionally boring—one target, no exotic options—so students can read it as a template for their own host-friendly DSP ports.
