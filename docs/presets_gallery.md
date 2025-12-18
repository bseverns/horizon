# Preset gallery: bus moves in 12 sketches

Treat this like a studio notebook: each preset is a one-line intent, a “feed me this” input hint, and a reminder to capture before/after WAVs so you can hear what actually changed. Tweak the numbers to taste—these are stage directions, not commandments.

## How to audition quickly
1. Route a loop through Horizon on a stereo track.
2. Bounce 4–8 bars *dry* and *with the preset*, labeling them `presetname_before.wav` / `presetname_after.wav` in `docs/audio/` (or next to your DAW session). Keep levels matched so you’re hearing tone, not loudness hype.
3. If something magical happens, jot settings in the comments section of your DAW or tack a note onto this file.

## Presets at a glance

| Preset | Intent in one sentence | Feed it this | Before/after WAV idea |
| --- | --- | --- | --- |
| Drum bus: Tight room glue | Subtle width bloom with light tape grit to make close mics feel like one kit. | Dry-ish multi-mic drums, little to no room verb. | Print 8 bars of the kit before/after with the same limiter target. |
| Drum bus: Smash then tuck | Aggressive transient shaving and narrowed sides for parallel crunch underneath the clean kit. | Overhead-heavy drums or loops with hot cymbals. | Bounce the crushed return solo vs blended at ~30% mix. |
| Bass: Anchor and air | Keep subs mono while adding a hair of top sheen so the bass speaks on small speakers. | DI bass or 808s that vanish on laptops. | Capture one bar of sustained notes; listen for midrange presence without widening the lows. |
| Vocals: Forward but polite | Mid tilt + gentle saturation to push the lead forward without sibilance spikes. | Lead vocal with consonant bite but low body. | Print a verse line; match loudness and check “s” control. |
| Guitars: Spread the stack | Subtle width expansion with transient taming so double-tracks feel wide yet glued. | L/R doubled electrics or acoustics. | Bounce a chorus chunk; note how palm mutes stay centered. |
| Pads: Floating halo | Side air boost and soft drive so pads shimmer without losing center gravity. | Stereo synth pads with mild modulation. | Record a sustained chord; the after file should feel like a mist, not a smear. |
| Synth leads: Center stage | Keep width modest and add limiter headroom so the lead can sit atop busy drums. | Mono or narrow synth leads fighting the beat. | Bounce a hook lick; check that attacks stay sharp but not piercing. |
| FX/Resample: Grainy widen | Push dirt and width for ear-candy layers that sit behind the vocal. | Glitched chops, foley hits, or risers. | Print stabs before/after; ensure transients aren’t disappearing entirely. |
| Mix bus: Polite polish | A gentle 1–2 dB lift of sheen and width with conservative limiting for “demo ready” bounce. | Full mix that already balances; this is not a rescue preset. | Export the whole chorus; aim for <2 dB gain reduction on the limiter. |
| Podcast/Voiceover: Present + steady | Mono-safe focus with transient control so the voice stays locked and level. | Spoken word, podcast roundtables, VO. | Bounce 30 seconds; listen for breaths smoothing without pumping. |
| Keys: Hammer haze | Slight transient shave plus width for percussive keys (Rhodes, Wurli) so they sit behind vocals. | DI electric keys with sharp attacks. | Print a verse comp; ensure bell tone still cuts while the body spreads. |
| Live bus safety: Latency-aware | Minimal processing, low lookahead, just enough ceiling to keep PA calm. | Stage mixes, monitor sends, anything where latency hurts. | Capture a monitor feed tap; compare timing feel with/without. |

## Notes on dialing these in
- **Width vs Dyn Width**: Use Width for static stereo size, Dyn Width to make hits tuck and tails bloom. If drums feel like a yo-yo, back off Dyn Width first.
- **Air vs Tilt**: Air is for shimmer, Tilt for general warmth/brightness. Stack lightly; small moves stay musical.
- **Limiter ceiling**: Start around −1 dBFS. If the limiter is chewing more than 3 dB on peaks, address the source mix before blaming the plugin.
- **Mix knob**: Parallel everything. If a move feels too obvious, back Mix to 60–80% instead of undoing the tone.

If you drop real WAVs into `docs/audio/`, link them next to the presets above so others can A/B instantly. Teaching is easier when everyone hears the same thing.
