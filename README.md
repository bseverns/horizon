# HORIZON — Mid/Side Spatial & Dynamics Station (Space)

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