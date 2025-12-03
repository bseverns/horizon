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

### Why these moves

- [Mid/side matrix](docs/block_notes.md#msmatrix) — adds/subtracts only to pivot between L/R and M/S without touching gain so the downstream moves stay honest.
- [Tilt](docs/block_notes.md#tilteq) — one-pole slant around ~1 kHz for quick warm/bright nudges; clamped to a sensible ±6 dB/oct so you can ride it like a tone knob.
- [Air shelf](docs/block_notes.md#aireq) — single split shelf that opens cymbals without phase weirdness; the pole itself keeps the top end smooth even when you twist fast.
- [Dyn width](docs/block_notes.md#dynwidth) — narrows hits, blooms tails, and low-anchors the sides so subs stay glued while the stage breathes.
- [Limiter](docs/block_notes.md#limiterlookahead) — short-delay clamp with a tilted detector and adaptive release so bright hits get caught early and sustain floats; wet/dry/bypass all ride the same delay for phase-honest mix moves.
- [Soft sat](docs/block_notes.md#softsaturation) — tanh grit mapped to ~1–10× drive with auto-normalization, meant as polite tape-ish hair rather than a splatter box.

Block-by-block intent + clamp/smoothing cheats live in [`docs/block_notes.md`](docs/block_notes.md).

The limiter runs its own delay line so dry/wet and bypass crossfades stay phase-honest. Detector tilt is detector-only.

## Platform
Teensy 4.x + SGTL5000 (Teensy Audio Library), 44.1 kHz / 128‑sample blocks.

## Quick Start
- Open `examples/minimal/minimal.ino` in Arduino + TeensyDuino.
- Select Teensy 4.0/4.1 and upload.
- Feed stereo program and try presets (see CSV + JSON).
- Feeling adventurous? Jump straight to `examples/preset_morph/preset_morph.ino`
  to hear slow-motion morphs between a cinema-wide wash and a gentle bus chain.

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
- Width lives in **0.0..1.0**. Static width is clamped there, and the dynamic width block
  only breathes inside that window so pots/encoders don’t promise "1.5x" magic that
  never actually happens.
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
- `examples/preset_morph/preset_morph.ino` — hands-free tour of two contrasting presets:
  a slow morph to cartoonishly wide stages, then a pillow-soft mastering chain.

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
