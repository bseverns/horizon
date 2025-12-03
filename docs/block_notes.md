# Core block notes

Each block here is a musician-facing cheat sheet: what the stage move is, what the knobs really do, and how we keep them from biting you (clamps, smoothing, and other guardrails).

## MSMatrix
Mid/side encoder/decoder that never touches gain—just adds and subtracts to pivot the mix between M/S and L/R. There are no parameters to smooth or clamp; the class exists to keep the math legible and reusable.

## TiltEQ
One-pole tilt around ~1 kHz for quick warm/bright slants. Tilt in dB/oct is clamped to −6..+6 before being mapped to paired shelves; the shared pole smooths the split itself, but there’s no parameter smoothing beyond that clamp, so changes land immediately.

## AirEQ
High-air shelf built from a single low-pass split so cymbals open up without phase weirdness. Frequency is clamped to 4–16 kHz and gain to −6..+6 dB when set; the pole handles signal smoothing, while parameter jumps take effect right away.

## DynWidth
Dynamic stereo width that narrows on hits and blooms on tails with a low-frequency anchor to keep subs mono-ish. Base width and dynamic amount are clamped 0..1, low-anchor frequency clamps 40–250 Hz, and the transient driver is clamped 0..1 per sample; the side low-pass uses a one-pole smoother set by the anchored cutoff so width swings feel like stagecraft instead of stutters.

## TransientDetector
Hit sniffer: rectifies the input, runs dual 2 ms / 80 ms envelope times, and spits out a 0..1 “activity” pulse. Sensitivity is clamped 0..1; attack/release coefficients are rebuilt on sample-rate changes; activity output is thresholded and clamped to 0..1 so downstream blocks get a chilled, smoothed control signal.

## SoftSaturation
Tanh drive meant to feel like polite tape grit. Amount clamps 0..1 and maps to ~1–10× drive; normalization by 1/tanh(drive) keeps level honest. There’s no dedicated smoother—changes hit instantly—but the tanh curve keeps transients rounded instead of splattered.

## LimiterLookahead
Short-delay limiter with tilted detector, adaptive release, and safety clipper. Ceiling clamps −12..−0.1 dBFS, release 20–200 ms, lookahead 1–8 ms, detector tilt −3..+3 dB/oct, mix 0..1, and bypass crossfades over ~5 ms; transient-driven release re-computes each block, and the gain envelope only moves faster than it relaxes so fader rides stay liquid. Safety tanh + hard clamp catch overs after the envelope, and dry/wet/bypass all ride the same delay so tweaks stay phase-honest.
