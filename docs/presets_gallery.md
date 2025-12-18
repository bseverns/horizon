# Preset gallery: bus moves in 12 sketches

Treat this like a studio notebook: each preset is a one-line intent, a “feed me this” input hint, and a reminder to capture before/after WAVs so you can hear what actually changed. Tweak the numbers to taste—these are stage directions, not commandments.

## How to audition quickly
1. Route a loop through Horizon on a stereo track.
2. Bounce 4–8 bars *dry* and *with the preset*, labeling them `presetname_before.wav` / `presetname_after.wav` in `docs/audio/` (or next to your DAW session). Level-match so you’re hearing tone, not loudness hype.
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
| Mix bus: Polite polish | A gentle lift of sheen and width with conservative limiting for “demo ready” bounce. | Full mix that already balances; this is not a rescue preset. | Export the whole chorus; aim for <2 dB gain reduction on the limiter. |
| Podcast/Voiceover: Present + steady | Mono-safe focus with transient control so the voice stays locked and level. | Spoken word, podcast roundtables, VO. | Bounce 30 seconds; listen for breaths smoothing without pumping. |
| Keys: Hammer haze | Slight transient shave plus width for percussive keys so they sit behind vocals. | DI Rhodes/Wurli with sharp attacks. | Print a verse comp; ensure bell tone still cuts while the body spreads. |
| Live bus safety: Latency-aware | Minimal processing, short lookahead, just enough ceiling to keep the PA calm. | Stage mixes, monitor sends, anything where latency hurts. | Capture a monitor feed tap; compare timing feel with/without. |

---

## Suggested starting controls (numerical cheats)

These numbers are launchpads. Ride **Mix** first if a move feels heavy.

**Unit sanity (because hosts disagree):**
- Width / Dyn Width / Transient / Dirt are **0–1**. :contentReference[oaicite:1]{index=1}  
- Tilt and Detector Tilt are **dB/oct**. :contentReference[oaicite:2]{index=2}  
- Air Freq is shown here in **kHz** (if your UI reads Hz, multiply by 1000). :contentReference[oaicite:3]{index=3}  
- Mix is shown here as a **percent** (if your UI reads 0–1, divide by 100). :contentReference[oaicite:4]{index=4}  
- Ceiling is **dBFS**; Release/Lookahead are **ms**. :contentReference[oaicite:5]{index=5}  

### A) Core “tone + width” settings

| Preset | Width | Dyn Width | Transient | Tilt | Air (Freq / Gain) | Low Anchor | Dirt | Ceiling | Mix | Out Trim |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Drum bus: Tight room glue | 0.62 | 0.25 | 0.40 | +0.3 | 9 kHz / +2 dB | 90 Hz | 0.12 | −1.0 dB | 75% | 0 dB |
| Drum bus: Smash then tuck | 0.45 | 0.60 | 0.65 | −0.5 | 8 kHz / +1 dB | 120 Hz | 0.35 | −2.0 dB | 35% | −1 dB |
| Bass: Anchor and air | 0.35 | 0.15 | 0.35 | +0.5 | 7 kHz / +1 dB | 80 Hz | 0.10 | −1.0 dB | 85% | 0 dB |
| Vocals: Forward but polite | 0.55 | 0.20 | 0.45 | +1.0 | 10 kHz / +1.5 dB | 100 Hz | 0.08 | −1.5 dB | 80% | 0 dB |
| Guitars: Spread the stack | 0.72 | 0.30 | 0.55 | −0.2 | 9 kHz / +2 dB | 110 Hz | 0.18 | −1.0 dB | 78% | 0 dB |
| Pads: Floating halo | 0.80 | 0.15 | 0.25 | +0.4 | 12 kHz / +3 dB | 90 Hz | 0.20 | −1.0 dB | 70% | −0.5 dB |
| Synth leads: Center stage | 0.50 | 0.20 | 0.50 | +0.3 | 9 kHz / +1 dB | 110 Hz | 0.12 | −1.0 dB | 85% | 0 dB |
| FX/Resample: Grainy widen | 0.85 | 0.40 | 0.60 | −0.3 | 11 kHz / +2.5 dB | 95 Hz | 0.45 | −3.0 dB | 50% | −1 dB |
| Mix bus: Polite polish | 0.60 | 0.20 | 0.40 | +0.2 | 12 kHz / +1.5 dB | 100 Hz | 0.10 | −1.0 dB | 90% | 0 dB |
| Podcast/Voiceover: Present + steady | 0.25 | 0.10 | 0.55 | +0.5 | 7 kHz / +0.5 dB | 120 Hz | 0.05 | −1.0 dB | 100% | 0 dB |
| Keys: Hammer haze | 0.70 | 0.25 | 0.50 | −0.1 | 9 kHz / +1.5 dB | 100 Hz | 0.15 | −1.0 dB | 75% | 0 dB |
| Live bus safety: Latency-aware | 0.55 | 0.10 | 0.40 | 0.0 | 9 kHz / +0.5 dB | 100 Hz | 0.05 | −0.5 dB | 100% | 0 dB |

### B) Limiter tweaks (leave everything else at plugin defaults)

Horizon exposes limiter **Release**, **Lookahead**, **Detector Tilt**, **Limit Mix**, and **Limit Link**. :contentReference[oaicite:6]{index=6}  
If you don’t touch these, you’re still using the limiter—just with the default feel.

| Preset | Lookahead (ms) | Release (ms) | Detector Tilt (dB/oct) | Limit Mix (0–1) | Limit Link |
| --- | --- | --- | --- | --- | --- |
| Drum bus: Tight room glue | default | default | default | 1.0 | Linked |
| Drum bus: Smash then tuck | default | default | +0.5 | 1.0 | Linked |
| Bass: Anchor and air | default | default | default | 1.0 | Linked |
| Vocals: Forward but polite | default | default | +0.3 | 1.0 | Linked |
| Guitars: Spread the stack | default | default | default | 1.0 | Linked |
| Pads: Floating halo | default | default | default | 1.0 | Linked |
| Synth leads: Center stage | default | default | default | 1.0 | Linked |
| FX/Resample: Grainy widen | default | default | +0.7 | 1.0 | Mid/Side |
| Mix bus: Polite polish | default | default | default | 1.0 | Linked |
| Podcast/Voiceover: Present + steady | default | default | default | 1.0 | Linked |
| Keys: Hammer haze | default | default | default | 1.0 | Linked |
| Live bus safety: Latency-aware | 2–3 | 40–60 | default | 1.0 | Linked |

---

## Notes on dialing these in
- **Width vs Dyn Width**: Use Width for static stereo size, Dyn Width to make hits tuck and tails bloom. If drums feel like a yo-yo, back off Dyn Width first.
- **Air vs Tilt**: Air is shimmer on the sides; Tilt is overall warmth/brightness in the mid. Stack lightly; small moves stay musical.
- **Limiter ceiling**: Start around −1 dBFS. If the limiter is chewing more than ~3 dB on peaks, fix the source mix before blaming the plugin.
- **Limit Mix vs Mix**: Limit Mix is limiter-only parallel; Mix is global wet/dry for the whole chain. :contentReference[oaicite:7]{index=7}
- **Mix knob**: Parallel everything. If a move feels too obvious, back Mix to 60–80% instead of undoing the tone.

If you drop real WAVs into `docs/audio/`, link them next to the presets above so others can A/B instantly. Teaching is easier when everyone hears the same thing.
