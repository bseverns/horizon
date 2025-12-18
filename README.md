# HORIZON — Mid/Side Spatial & Dynamics Station

A mastering‑style “space shaper”: mid/side encode → tone shaping → **dynamic width** via transient detection → soft clip → lookahead limiter → width/mix.
Makes dense mixes breathe (tails wider, hits focused). Drop it after any instrument.

## Signal flow (ASCII cheat sheet)
```
[I2S In] → MS encode
          → Mid tilt (pivot ~1 kHz) → Side air shelf → DynWidth (transient-following)
          → MS decode → Lookahead limiter (tilted detector, adaptive release)
          → SoftSat (mild post safety) → Output trim → [I2S Out]
```
- Block-by-block intent + clamp/smoothing cheats live in [`docs/block_notes.md`](docs/block_notes.md).
- The limiter runs its own delay line so dry/wet and bypass crossfades stay phase-honest. Detector tilt is detector-only.

## How to navigate this notebook
- **Instant context** → start with a platform quick start (Teensy or laptop) to hear the chain.
- **Deep dive** → skim the “Why it sounds this way” notebook chunks near the bottom, then hop into `docs/block_notes.md`.
- **Host curious** → the desktop quick start below mirrors the Teensy flow so you can keep the embedded mindset while in DAW land.

## Where to start
- **Teensy / hardware:** flash the minimal sketch, twist pots, watch the serial scope breathe.
- **Desktop / host:** build the CMake preset, render a file through `horizon-cli`, then open the plugin to see every knob → setter map.

## Teensy quick start (hardware-first)
- Open `examples/minimal/minimal.ino` in Arduino + TeensyDuino.
- Select Teensy 4.0/4.1 and upload.
- Feed stereo program and try presets (see CSV + JSON).
- Feeling adventurous? Jump straight to `examples/preset_morph/preset_morph.ino` to hear slow-motion morphs between a cinema-wide wash and a gentle bus chain.
- Need visuals? Flash `examples/horizon_scope/horizon_scope.ino` and watch width/transient/GR scroll by at 115200 baud.

## Desktop quick start (CLI + presets + plugin map)
This is the laptop twin of the Teensy quick start—same smoothing and guardrails, just wrapped for CMake instead of Arduino. Treat it like a studio notebook page that shows how the embedded discipline survives on the host.

1) **Build the host preset**
- CMake presets bake in sane defaults so you can configure + build without spelunking flags:
  ```bash
  cmake --preset linux-clang
  cmake --build --preset linux-clang
  cmake --install cmake-out/linux-clang --prefix /where/you/want/it
  ```
  - macOS Universal 2: `cmake --preset macos-universal-release` → `cmake --build --preset macos-universal-release`
  - Windows 11 + MSVC: `cmake --preset windows-msvc-release` → `cmake --build --preset windows-msvc-release`
- Prefer raw CMake? `cmake -S . -B cmake-build -DCMAKE_BUILD_TYPE=Release` still works. `sync_compile_commands` rides along as an always-on helper target for clangd/IntelliSense vibes.
- Build trees stay out of the repo on purpose: `cmake-out/` and any `cmake-build*/` scratch pads are ignored alongside the usual `CMakeFiles/`, `CMakeCache.txt`, `CMakeUserPresets.json`, installer manifests, and generated `compile_commands.json`. Toss them at will; the tracked presets remain the source of truth.

2) **Run the CLI like you would a Teensy preset**
- Render a file with a preset baked in:
  ```bash
  ./cmake-out/linux-clang/horizon_cli input.wav output.flac --preset bus_glue
  ./cmake-out/linux-clang/horizon_cli --list-presets
  ```
- Want the serial-scope feel while you bounce? Add `--scope` to log the block-level width, transient pulse, and limiter GR. Example line:
  ```
  [scope] f0 | W 0.64 [=============....] | T 0.21 [++........] | GR -2.3 dB [#####.......]
  ```
  Same telemetry as the Teensy serial scope, just breathing through stdio instead of USB.

3) **Map the plugin knobs (DAW view)**
- Build the JUCE target (VST3 + standalone):
  ```bash
  cmake -S plugins -B plugins/build
  cmake --build plugins/build
  ```
- Every control in the editor forwards directly to a `HostHorizonProcessor` setter:
  - Width → `setWidth` (0 = mono, 1 = max)
  - Dyn Width → `setDynWidth` (transients tug sides in before tails reopen)
  - Transient → `setTransientSens`
  - Tilt → `setMidTilt`
  - Air Freq/Gain → `setSideAir(freq, gain)`
  - Low Anchor → `setLowAnchor`
  - Dirt → `setDirt`
  - Ceiling / Release / Lookahead / Detector Tilt / Limit Mix / Link → limiter setters (`setCeiling`, `setLimiterReleaseMs`, `setLimiterLookaheadMs`, `setLimiterDetectorTilt`, `setLimiterMix`, `setLimiterLinkMode`)
  - Mix → `setMix`, Out Trim → `setOutputTrim`
- Hover text in the plugin matches these names so students can trace knob → setter → mix move without mystery glue. Full map + build quirks live in [`plugins/README.md`](plugins/README.md).

### Where builds land (so you can find the bits you just compiled)
- **CMake presets** drop CLI + library binaries under `cmake-out/<preset>/` (e.g. `cmake-out/linux-clang/`).
- **Raw `cmake -B cmake-build`** keeps everything in the `cmake-build/` tree if you’d rather skip presets.
- **JUCE plugin builds** live in `plugins/build`, with JUCE’s defaults giving you `plugins/build/VST3/` and `plugins/build/Standalone/` drop points. Need deeper path spelunking? Peek at [`plugins/README.md`](plugins/README.md) for the nitty-gritty.

## Platform
Teensy 4.x + SGTL5000 (Teensy Audio Library), 44.1 kHz / 128‑sample blocks. Host builds reuse the exact DSP core via `HostHorizonProcessor` so demos, CI, and DAWs all share the same brain.

## PlatformIO, CI, and host tricks
Quick map (details live in the sections below):

- **Environments**
  - `main_teensy40` / `main_teensy41` — stage-ready firmware; flash with `pio run -e main_teensy41 -t upload`.
  - `scope_teensy40` / `scope_teensy41` — telemetry-on-Serial builds for block meters; upload then watch at 115200.
  - `native_dsp` — host-only math bench; run `pio test -e native_dsp` (see [Native DSP test bench](#native-dsp-test-bench-no-hardware-required)).

- **Core commands**
  - Build/upload: `pio run -e main_teensy41 -t upload` (swap envs as needed).
  - Serial scope: `pio run -e scope_teensy41 -t upload` then crack open a terminal at 115200.
  - Tests (no hardware): `pio test -e native_dsp`.
  - Lint/compile DB: `HORIZON_LINT_STUBS=1 pio run -e main_teensy41 -t compiledb` (see [Host-side IntelliSense / clangd cheat codes](#host-side-intellisense--clangd-cheat-codes)).

- **CI coverage**
  - GitHub Actions installs PlatformIO, builds every Teensy env, and runs `native_dsp` tests so broken flags or platform-only includes get caught early.

### Native DSP test bench (no hardware required)
- Want to bash on the DSP math without a Teensy plugged in? Run `pio test -e native_dsp` to spin up a host-only build that links tiny Arduino/Audio stubs and exercises the limiter, smoother, and width logic.
- The env doesn’t inherit any Teensy/Arduino scaffolding, so the native toolchain stays lean and never nags for a board definition—perfect for CI runners and students poking around on a laptop.
- The project-level `test_dir = test/native_dsp` forces PlatformIO to scoop up the host bench directly, and `test_build_src = yes` keeps the DSP implementation compiled alongside the tests even when firmware entry points are filtered out. Great for CI, teaching, or proving a refactor didn’t sandbag the groove.
- There’s also a tiny WAV harness (`test/native_dsp/process_wav.cpp`) with a callable `horizon_wav_driver` so you can bounce audio through Horizon on the host. The default `native_dsp` run sticks to the Unity test main to avoid dueling entry points, but you can flip on `HORIZON_WAV_STANDALONE` if you want a quick command-line renderer instead of tests.

### Host-side IntelliSense / clangd cheat codes
- Linting your editor without dragging in the whole Teensy core? The `patches/cores/teensy4/lint_stubs.h` shim is opt-in so firmware builds always pull `F_CPU_ACTUAL`, `NVIC_SET_PENDING`, etc. from the real PJRC headers. Flip `HORIZON_LINT_STUBS=1` when generating editor metadata and the shim injects no-op definitions for the usual suspects (`__disable_irq`, `Serial`, ...), keeping clangd/IntelliSense chill even if the PlatformIO download cache is missing.
- Preferred compile database: `HORIZON_LINT_STUBS=1 pio run -e main_teensy41 -t compiledb` (drops `.pio/build/main_teensy41/compile_commands.json`).
- Offline/editor-only fallback: `python tools/gen_compile_commands.py` (writes `compile_commands.json` at the repo root with the same `-I patches/cores/teensy4` + `-include patches/cores/teensy4/lint_stubs.h` flags baked in).
- Point your editor at that database (VS Code already ships a `.vscode/c_cpp_properties.json` that hooks `compile_commands.json` and forces the lint shim include path) and re-index so host-side scanning stops flagging `F_CPU_ACTUAL`, `IRQ_SOFTWARE`, and `NVIC_SET_PENDING` as undefined.

## Control ranges (cheat sheet)
- Width lives in **0.0..1.0**. Static width is clamped there, and the dynamic width block only breathes inside that window so pots/encoders don’t promise "1.5x" magic that never actually happens.
- Limiter: ceiling clamps to **-12..-0.1 dBFS** across code, CSV, and this cheat sheet (no fake "0 dB" promises—leave a whisper of headroom), release 20..200 ms (adapts shorter on transient hits), lookahead 1..8 ms, detector tilt -3..+3 dB/oct, mix 0..1, link mode = Linked or Mid/Side, bypass is a 5 ms crossfade.

## Folders
- `src/` — core classes (matrix, EQs, detector, limiter, smoothing).
- `examples/` — minimal wiring sketch.
- `presets/` — starter presets JSON.
- `docs/` — control map CSV.

## License
MIT — see `LICENSE`.

## Examples
- `examples/minimal/minimal.ino` — bare wiring: I2S in → Horizon → I2S out.
- `examples/horizon_scope/horizon_scope.ino` — ASCII "scope" showing block width, transient activity and limiter gain.
- `examples/preset_morph/preset_morph.ino` — hands-free tour of two contrasting presets: a slow morph to cartoonishly wide stages, then a pillow-soft mastering chain.

## Limiter telemetry + LED ladder
- Gain reduction (dB) maps to an 8-step ladder at: **−1, −2, −3, −4, −6, −8, −10, −12 dB** (higher index = deeper clamp).
- Telemetry from `LimiterLookahead::Telemetry` gives per-block peak in/out plus GR dB via `getLimiterGRdB()` for quick logging or UI.
- Example serial line (0.5 s cadence): `GR(dB): -2.3 | Pin: 0.89 Pout: 0.79 | LEDs: 4 | Clip: no`

## Latency notes
- Audio path latency = **lookahead delay + I2S buffer**. Default ~5–6 ms lookahead keeps the limiter transparent; parallel mix and bypass are already delay-compensated inside the limiter.

### Making the limiter actually look ahead
- **Prime the delay**: call `LimiterLookahead::setup()` once at boot so the circular buffer is zeroed and the crossfade math is warmed up instead of spewing whatever was on the stack.
- **Pick a lead time**: set `setLookaheadMs(1..8)` to taste (5–6 ms is the sweet “clairvoyant fader ride” default). The setter converts ms → samples and clamps so we never outrun the buffer.
- **Stay on the delayed rails**: process audio through `processStereo`, which writes the live sample to the buffer, reads the delayed copy for output, and drives the detector off the undelayed feed so gain reduction lands before the delayed program hits your ears. Wet/dry and bypass already ride the same delay so phase stays honest.

## Why it sounds this way (studio notebook addenda)
- **Limiter lookahead math** — The limiter runs a short delay line for the program path but drives its envelope from a tilted detector so bright stuff pops the gain computer faster. Lookahead is set in milliseconds and turned into samples for a circular buffer; the detector uses linked peaks (or mid/side maxima) and a smoothed gain request that can only clamp faster than it releases. Release adapts: a transient average steers the time constant between a 10 ms-ish fast lane and a slower base release so cymbal sustains float while snares snap back. Wet/dry stays phase-aligned because both paths ride the same delay. Safety clipping sits after gain to catch rogue peaks but never replaces the envelope.
- **Transient activity curve** — The transient meter is a two-time-constant envelope follower (2 ms attack, 80 ms release) with a sliding threshold. Sensitivity 0..1 moves that threshold from about 0.05 to 0.5 of full-scale, and anything above it maps linearly to a 0..1 “activity” pulse. That pulse feeds dynamic width and limiter release decisions, meaning the harder the stick hit, the quicker the release rebounds and the more the stage narrows before blooming back out.
- **Width-to-mid/side mapping** — Dynamic width isn’t a random chorus trick: the side channel is split by a gentle low-pass anchored around 40–250 Hz so sub energy stays centered. Transient activity crossfades between two static widths: a narrowed-on-hit value and a widened tail. High band uses the chosen width directly; lows get an extra mono pull (25% of the chosen width) to keep kick/bass glued. Mid passes through untouched here, but the width fader swings are logged per sample so you can meter how the stage breathes.
