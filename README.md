# HORIZON — Mid/Side Spatial & Dynamics Station

A mastering‑style “space shaper”: mid/side encode → tone shaping → **dynamic width** via transient detection → soft clip → lookahead limiter → width/mix.
Makes dense mixes breathe (tails wider, hits focused). Drop it after any instrument.

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
