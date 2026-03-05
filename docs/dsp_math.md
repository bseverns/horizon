# Horizon DSP Math Notes

This page is the "under the hood" companion to `docs/block_notes.md`.
It captures the exact coefficient and transfer-function choices used in the current codebase.

Assumptions:
- Sample rate `fs = 44100 Hz` unless explicitly changed (`setSampleRate` paths).
- Audio is processed sample-by-sample inside 128-sample Teensy audio blocks.

## Shared helpers

- Decibel to linear:
  - `lin = 10^(dB / 20)`
- One-pole coefficient from cutoff frequency `f`:
  - `alpha = 1 - exp(-(2*pi*f)/fs)`
- First-order smoother update:
  - `y[n] = y[n-1] + alpha * (x[n] - y[n-1])`

## TiltEQ (`src/TiltEQ.cpp`)

Goal: symmetric low/high tilt around a ~1 kHz pivot.

1. Clamp control:
   - `tilt_dB_per_oct in [-6, +6]`
2. Map to shelf gains:
   - `totalShelf = 2 * tilt_dB_per_oct`
   - `half = totalShelf / 2`
   - `gLow = 10^(-half/20)`
   - `gHigh = 10^(+half/20)`
3. Split and recombine:
   - `low[n] = low[n-1] + alphaPivot * (x[n] - low[n-1])`
   - `high[n] = x[n] - low[n]`
   - `y[n] = gLow*low[n] + gHigh*high[n]`

Notes:
- Pivot is fixed at `~1000 Hz`.
- This is a musical shelf-pair mapping, not a strict analog tilt emulation.

## AirEQ (`src/AirEQ.cpp`)

Goal: side-channel high shelf via one-pole split.

1. Clamp controls:
   - `freqHz in [4000, 16000]`
   - `gainDb in [-6, +6]`
2. Build split:
   - `low[n] = low[n-1] + alphaAir * (x[n] - low[n-1])`
   - `high[n] = x[n] - low[n]`
3. Apply shelf:
   - `gHigh = 10^(gainDb/20)`
   - `y[n] = low[n] + gHigh*high[n]`

## TransientDetector (`src/TransientDetector.cpp`)

Goal: produce a stable `activity in [0, 1]` from signal level.

1. Envelope follower:
   - Attack time: `2 ms`
   - Release time: `80 ms`
   - `aAtk = 1 - exp(-1/(attackSec*fs))`
   - `aRel = 1 - exp(-1/(releaseSec*fs))`
   - `env[n] = env[n-1] + a*(abs(x[n]) - env[n-1])` where `a = aAtk` when rising, else `aRel`
2. Sensitivity threshold:
   - `sens in [0,1]`
   - `threshold = 0.05 + 0.45*sens`  (range ~0.05..0.5)
3. Activity mapping:
   - if `env <= threshold`: `activity = 0`
   - else: `activity = (env - threshold) / (1 - threshold)`
   - clamp to `[0,1]`

## DynWidth (`src/DynWidth.cpp`)

Goal: narrow on hits, widen on tails, keep lows centered.

1. Clamp controls:
   - `baseWidth in [0,1]`
   - `dynAmt in [0,1]`
   - `lowAnchorHz in [40,250]`
2. Split side channel:
   - `lowSide[n] = lowSide[n-1] + alphaAnchor*(side[n]-lowSide[n-1])`
   - `highSide[n] = side[n] - lowSide[n]`
3. Build dynamic width endpoints:
   - `widen = min(1, baseWidth + dynAmt)`
   - `narrow = max(0, baseWidth * (1 - 0.9*dynAmt))`
4. Crossfade with transient activity `t in [0,1]`:
   - `widthNow = narrow*t + widen*(1-t)`
5. Apply low-frequency anchor:
   - `lowScale = 0.25`
   - `sideOut = highSide*widthNow + lowSide*(widthNow*lowScale)`

## SoftSaturation (`src/SoftSaturation.cpp`)

Goal: gentle tanh drive with approximate unity normalization.

1. Clamp amount:
   - `amt in [0,1]`
2. Map drive:
   - `drive = 1 + 9*amt` (about 1x..10x)
3. Normalize:
   - `norm = 1 / tanh(drive)` (guarded for zero)
4. Process:
   - if `amt ~= 0`: `y=x`
   - else: `y = tanh(drive*x) * norm`

## LimiterLookahead (`src/LimiterLookahead.cpp`)

Goal: lookahead peak limiting with adaptive release and bypass-safe alignment.

### Parameter clamps
- Ceiling: `[-12, -0.1] dBFS`
- Release: `[20, 200] ms`
- Lookahead: `[1, 8] ms` (also clamped to internal delay buffer bounds)
- Detector tilt: `[-3, +3] dB/oct`
- Mix: `[0,1]`

### Lookahead delay

- Convert lookahead ms to samples:
  - `N = round(lookaheadMs * 0.001 * fs)`
- Circular buffer read:
  - write current sample at `writeIdx`
  - read dry-aligned sample at `readIdx = writeIdx - N` (wrapped)
- Both dry and wet paths use delayed samples to keep wet/dry and bypass phase-aligned.

### Detector and envelope

1. Detector feed:
   - apply detector tilt filters to `L` and `R`
   - linked mode: `level = max(abs(detL), abs(detR))`
   - mid/side mode:
     - `m = 0.5*(detL + detR)`
     - `s = 0.5*(detL - detR)`
     - `level = max(abs(m), abs(s))`
2. Required gain:
   - `gReq = min(1, ceilingLin / max(level, eps))`
3. Adaptive release coefficient:
   - `fastMs = max(10, 0.25*releaseMs)`
   - `slowMs = releaseMs`
   - `t = 1 - transientAvg`
   - `releaseNowMs = fastMs + (slowMs-fastMs)*t`
   - `aRel = 1 - exp(-1/(releaseNowSec*fs))`
4. Envelope update:
   - `relaxing = env + aRel*(1-env)`
   - `env = min(gReq, relaxing)`  (instant clamp, smooth release)

### Output stage

- Wet sample: delayed input multiplied by `env`
- Safety stage:
  - mild tanh soft clip (`SoftSaturation` at low amount)
  - hard clamp to `+-ceilingLin`
- Limiter wet/dry:
  - `mixed = dry + mix*(wet - dry)`
- Bypass crossfade:
  - 5 ms ramp on bypass toggles
  - `out = mixed*(1-bypassMix) + dry*bypassMix`

## ParamSmoother (`src/ParamSmoother.cpp`)

Simple one-pole target smoothing:

- `value[n] = value[n-1] + alpha*(target - value[n-1])`
- `alpha` clamped to `[0,1]`
- First call seeds `value` to avoid startup jumps.

Time-constant helper:
- `dt = samplesPerUpdate / fs`
- `tau = ms * 0.001`
- `alpha = 1 - exp(-dt/tau)`

