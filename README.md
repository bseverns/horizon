# HORIZON — Mid/Side Spatial & Dynamics Station

A mastering‑style “space shaper”: mid/side encode → tone shaping → **dynamic width** via transient detection → soft clip → lookahead limiter → width/mix.
Makes dense mixes breathe (tails wider, hits focused). Drop it after any instrument.

## Platform
Teensy 4.x + SGTL5000 (Teensy Audio Library), 44.1 kHz / 128‑sample blocks.

## Quick Start
- Open `examples/minimal/minimal.ino` in Arduino + TeensyDuino.
- Select Teensy 4.0/4.1 and upload.
- Feed stereo program and try presets (see CSV + JSON).

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

## Examples

- `examples/minimal/minimal.ino` — bare wiring: I2S in → Horizon → I2S out.
- `examples/horizon_scope/horizon_scope.ino` — ASCII "scope" showing block width, transient activity and limiter gain.
