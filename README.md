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

The limiter runs its own delay line so dry/wet and bypass crossfades stay phase-honest. Detector tilt is detector-only.

## Platform
Teensy 4.x + SGTL5000 (Teensy Audio Library), 44.1 kHz / 128‑sample blocks.

## Quick Start
- Open `examples/minimal/minimal.ino` in Arduino + TeensyDuino.
- Select Teensy 4.0/4.1 and upload.
- Feed stereo program and try presets (see CSV + JSON).
- Feeling adventurous? Jump straight to `examples/preset_morph/preset_morph.ino`
  to hear slow-motion morphs between a cinema-wide wash and a gentle bus chain.

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

## PlatformIO

- Use the provided `platformio.ini` at the repo root.
- `src/main.cpp` is the control-surface firmware with optional scope mode.
- Envs:
  - `main_teensy40` / `main_teensy41` — performance builds (no Serial scope).
  - `scope_teensy40` / `scope_teensy41` — enable `HORIZON_BUILD_SCOPE` and print width/transient/limiter bars over Serial.
- Build and upload, e.g.:
  - `pio run -e main_teensy41 -t upload`
  - `pio run -e scope_teensy41 -t upload`
- Linting your editor without dragging in the whole Teensy core? The
  `patches/cores/teensy4/lint_stubs.h` shim is now **opt-in** so firmware
  builds always pull `F_CPU_ACTUAL`, `NVIC_SET_PENDING`, etc. from the real
  PJRC headers. Flip `HORIZON_LINT_STUBS=1` when generating editor metadata
  and the shim injects no-op definitions for the usual suspects (`__disable_irq`,
  `Serial`, ...), keeping clangd/IntelliSense chill even if the PlatformIO
  download cache is missing.

### Host-side IntelliSense / clangd cheat codes

- Generate a compile database so clangd inherits the Teensy include + forced
  lint stubs:
  - Preferred: `HORIZON_LINT_STUBS=1 pio run -e main_teensy41 -t compiledb`
    (drops `.pio/build/main_teensy41/compile_commands.json`).
  - Offline/editor-only fallback: `python tools/gen_compile_commands.py`
    (writes `compile_commands.json` at the repo root with the same
    `-I patches/cores/teensy4` + `-include patches/cores/teensy4/lint_stubs.h`
    flags baked in).
- Point your editor at that database (VS Code already ships a
  `.vscode/c_cpp_properties.json` that hooks `compile_commands.json` and forces
  the lint shim include path).
- Re-index the workspace. Host-side scanning should stop flagging
  `F_CPU_ACTUAL`, `IRQ_SOFTWARE`, and `NVIC_SET_PENDING` as undefined because
  `lint_stubs.h` is now always forced in for non-Teensy toolchains.

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
