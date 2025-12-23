# Core block notes

Each block here is a musician-facing cheat sheet: what the stage move is, what the knobs really do, and how we keep them from biting you (clamps, smoothing, and other guardrails).

## MSMatrix
Mid/side encoder/decoder that never touches gain—just adds and subtracts to pivot the mix between M/S and L/R. There are no parameters to smooth or clamp; the class exists to keep the math legible and reusable.

**Patch recipe: Mid-safe spreader**  
*Wire before Tilt/Air when you want width with honest mono.*  
Range sketch: `Width 0.4 → 0.9` while keeping `Low Anchor ~100 Hz` downstream.  
Before/after spectra: center stays flat; only side band rises above ~500 Hz as width blooms.

## TiltEQ
One-pole tilt around ~1 kHz for quick warm/bright slants. Tilt in dB/oct is clamped to −6..+6 before being mapped to paired shelves; the shared pole smooths the split itself, but there’s no parameter smoothing beyond that clamp, so changes land immediately.

**Patch recipe: Warm the mid without dunking cymbals**  
Set `Tilt` between `+0.3..+1.5 dB/oct` and keep `Air` modest (< +2 dB) so the shelves sum gracefully.  
Spectrum sketch: lows lift gently below 500 Hz, highs slope down after 2–3 kHz; watch a before/after FFT to show the pivot staying near 1 kHz.

## AirEQ
High-air shelf built from a single low-pass split so cymbals open up without phase weirdness. Frequency is clamped to 4–16 kHz and gain to −6..+6 dB when set; the pole handles signal smoothing, while parameter jumps take effect right away.

**Patch recipe: Air vs fizz lab**  
Park `Freq` at `8–10 kHz` for vocals, `12–14 kHz` for pads; keep `Gain` in `+1..+3 dB` until you can see the shelf knee in a spectrum analyzer.  
Before/after spectra: high shelf lifts only the top octave, leaving mids untouched; show students how pushing past +4 dB starts to bite.

## DynWidth
Dynamic stereo width that narrows on hits and blooms on tails with a low-frequency anchor to keep subs mono-ish. Base width and dynamic amount are clamped 0..1, low-anchor frequency clamps 40–250 Hz, and the transient driver is clamped 0..1 per sample; the side low-pass uses a one-pole smoother set by the anchored cutoff so width swings feel like stagecraft instead of stutters.

**Patch recipe: Bus glue vs wash**  
Start with `Width 0.55`, `Dyn Width 0.2`, `Low Anchor 120 Hz` for “bus glue”; slide toward `Width 0.9`, `Dyn Width 0.5`, `Low Anchor 90 Hz` to make a wash.  
Before/after spectra: mids stay centered under 150 Hz while side energy above 500 Hz breathes more as Dyn Width increases—perfect scope fodder with `block_width` telemetry.

## TransientDetector
Hit sniffer: rectifies the input, runs dual 2 ms / 80 ms envelope times, and spits out a 0..1 “activity” pulse. Sensitivity is clamped 0..1; attack/release coefficients are rebuilt on sample-rate changes; activity output is thresholded and clamped to 0..1 so downstream blocks get a chilled, smoothed control signal.

**Patch recipe: Teach pumping without panic**  
Keep `Sensitivity 0.35..0.6`; route its output to Dyn Width and watch telemetry to show how kicks briefly narrow sides.  
Before/after spectra: low-end center stays fixed, while side content dips on hits then recovers—overlay envelopes to show smoothing in action.

## SoftSaturation
Tanh drive meant to feel like polite tape grit. Amount clamps 0..1 and maps to ~1–10× drive; normalization by 1/tanh(drive) keeps level honest. There’s no dedicated smoother—changes hit instantly—but the tanh curve keeps transients rounded instead of splattered.

**Patch recipe: “Tape whisper” lane**  
Live between `Amount 0.05..0.25` on mix busses; feed a snare stem and show how odd-order fuzz shows up above 2 kHz without smashing the low end.  
Before/after spectra: subtle harmonic bumps at 2–6 kHz, peak level held steady by the built-in normalization.

## LimiterLookahead
Short-delay limiter with tilted detector, adaptive release, and safety clipper. Ceiling clamps −12..−0.1 dBFS, release 20–200 ms, lookahead 1–8 ms, detector tilt −3..+3 dB/oct, mix 0..1, and bypass crossfades over ~5 ms; transient-driven release re-computes each block, and the gain envelope only moves faster than it relaxes so fader rides stay liquid. Safety tanh + hard clamp catch overs after the envelope, and dry/wet/bypass all ride the same delay so tweaks stay phase-honest.

**Patch recipe: Link-mode clinic**  
Run hot drum loops at `Ceiling -2 dB`, `Lookahead 5–6 ms`, `Release 70–120 ms`, and flip `Limit Link` between **Linked** and **Mid/Side** while watching `limiter_gain`.  
Before/after spectra: identical tonal balance, but Mid/Side linking lets side tails regain level sooner—capture both in `docs/audio/` so students can A/B coupling philosophy.
