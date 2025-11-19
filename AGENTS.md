# AGENTS.md

Guidelines for humans and machine collaborators working on **Horizon** and related audio tools.

This repository is part of a small ecosystem of real-time DSP instruments and utilities built for
**Teensy 4.x + Audio Shield**, with an emphasis on:

- Legible, modular DSP blocks (small classes, clear responsibilities)
- Real-time safety (no surprises in the audio thread)
- Pedagogical value (students should be able to read and learn from this code)
- Musical usefulness over mathematical purity

If you are an automated agent (LLM, code generator, refactoring bot, etc.), this document is your
contract with the project.

---

## 1. Scope & Intent

- **Primary role of this repo**  
  A stereo bus / master-adjacent processor (`AudioHorizon`) plus its internal DSP utilities:
  mid/side matrixing, tone shaping, dynamics of stereo width, soft saturation, and limiting.

- **Intended environment**
  - Teensy 4.0 / 4.1
  - PJRC Audio Library (44.1 kHz, block-based processing)
  - Teensy Audio Shield (SGTL5000) for most examples

- **Secondary role**  
  A teaching artifact: code is meant to be **read aloud**, diagrammed on whiteboards,
  and adapted into new machines (siblings like FOG BANK, ORBIT LOOPER, etc.).

---

## 2. Design Principles

When touching this codebase, prefer:

1. **Clarity over cleverness**
   - Short, single-purpose functions.
   - Descriptive names over abbreviations, unless the domain demands them (e.g. `dB`, `Hz`, `ms`).

2. **Composable blocks**
   - DSP building blocks live in `src/` as small classes with `.h` + `.cpp`.
   - Each block should be usable independently in other projects.

3. **Real-time safety**
   - No dynamic allocation (`new`, `malloc`, `std::vector::push_back`) inside `update()` or any
     function called from the audio callback.
   - No blocking I/O (Serial printing, SD, filesystem) from `update()`.
   - No locks or mutexes on the audio path.

4. **Gentle, musical behavior**
   - Parameters are smoothed in time (`ParamSmoother` or equivalent).
   - Defaults should “sound safe” on a stereo mix (no immediate clipping or extreme artifacts).
   - Where possible, favor continuity over discontinuity (no sudden jumps in gain/phase).

5. **Pedagogical structure**
   - Code should be annotated enough that an intermediate C++ / audio student can follow it.
   - Complex math should be commented with the *why*, not just the *what*.

---

## 3. Project Layout & Responsibilities

**Core DSP blocks (examples, non-exhaustive):**

- `MSMatrix` – mid/side encode/decode helpers.
- `TiltEQ` – simple tilt EQ around a pivot frequency.
- `AirEQ` – high-frequency shelf via one-pole split.
- `DynWidth` – stereo width shaping with low-frequency anchoring.
- `TransientDetector` – envelope follower + transient activity metric.
- `SoftSaturation` – tanh-based saturation with normalized drive.
- `LimiterLookahead` – peak limiter (currently zero-latency with fast attack and smooth release).
- `ParamSmoother` – simple one-pole parameter smoothing.

**Top-level processor:**

- `AudioHorizon : AudioStream`
  - Owns instances of the above blocks.
  - Exposes a small set of high-level controls:
    - `setWidth`, `setDynWidth`, `setTransientSens`
    - `setMidTilt`, `setSideAir`, `setLowAnchor`
    - `setDirt`, `setCeiling`, `setMix`
  - Provides per-block telemetry:
    - `getBlockWidth()`
    - `getBlockTransient()`
    - `getLimiterGain()`

**Examples:**

- `examples/` contains sketches for:
  - Minimal wiring / smoke tests.
  - “Scope” style telemetry (printing width / transient / limiter data to Serial).

Agents: when adding new functionality, follow these patterns:
- New DSP concept → new class in `src/` (`FooBlock.h/.cpp`) with a narrow, testable API.
- New teaching/demo idea → new `examples/<name>/<name>.ino` or `src/main.cpp` (for PlatformIO).

---

## 4. Coding Standards & Conventions

**Language & style**

- C++14ish subset, as supported by Teensy’s Arduino toolchain and PlatformIO’s `teensy` platform.
- Stick to the existing style:
  - `CamelCase` for class names (`SoftSaturation`).
  - `snake_case` for private members (`_attackCoeff`, `_telemetryWidth`).
  - `setSomething(...)` for mutating configuration.
  - `processSample`, `processStereo`, or `update` for processing calls.

**Headers vs Implementation**

- Headers contain:
  - Class declarations.
  - Short inline helpers only when trivially simple.
- `.cpp` files contain:
  - All non-trivial implementation.
  - `static inline` utility functions (e.g. `clampf_*`) local to that translation unit.

**Units**

- Use explicit units in names where ambiguity exists:
  - `freqHz`, `gainDb`, `attackSec`, `lowAnchorHz`.
- Document expected ranges in comments (`0..1`, `-12..+12 dB`, etc.).

---

## 5. Real-Time & Safety Rules for Agents

If you are an automated tool modifying DSP code, **you MUST observe these rules**:

1. **Do NOT** add:
   - `new`, `delete`, `malloc`, `free`, or any STL containers that allocate, inside audio callbacks.
   - `Serial.print` (or logging) inside `update()` or other hot paths.
   - File I/O, SD access, `delay()`, or blocking operations in `update()`.

2. **Do NOT** change:
   - The public API of `AudioHorizon` (its `setX()` and `getX()` methods) without leaving clear
     comments and, ideally, updating accompanying documentation and examples.
   - The expected ranges of parameters without updating comments and any control map docs.

3. **DO**:
   - Use `ParamSmoother` or equivalent when introducing new continuous controls that affect gain,
     frequency, or time constants.
   - Keep processing loops tight and branch-light; avoid unnecessary per-sample conditionals
     where a block-level decision will do.

4. **Latency & Phase**
   - If a new block introduces latency or significant phase changes, document it clearly and think
     about its placement in the chain.
   - Do not silently insert delay lines into the main signal path without explaining why.

---

## 6. Documentation & Pedagogy

For humans and agents:

- When implementing a new block or significantly changing behavior:
  - Add a brief doc-style comment block at the top of the `.cpp` describing:
    - What the block does in musical terms.
    - Any important parameters and their ranges.
    - Any non-obvious math.

- Keep **examples** in sync:
  - If you add a parameter, update at least one example to exercise it.
  - Examples should compile and run on current Teensy/PlatformIO configurations.

- When in doubt, **err on the side of over-commenting** algorithmic intent.

---

## 7. PlatformIO & Environments

This project can be built via PlatformIO with multiple envs, e.g.:

- `main_teensy40`, `main_teensy41` – performance/“real use” builds.
- `scope_teensy40`, `scope_teensy41` – debugging/teaching builds with telemetry enabled.

Agents:
- If you add new `#ifdef`-guarded behaviors (e.g. `HORIZON_BUILD_SCOPE`), keep the number of
  configuration flags small and clearly named.
- Avoid scattering `#ifdef`s throughout DSP code; prefer:
  - Small, well-named helper functions.
  - A single conditional in the outer `loop()` or top-level app code.

---

## 8. Extending the Suite

When adding new processors or siblings (e.g. other dynamics tools, spectral toys, feedback devices):

1. **Reuse core blocks** where appropriate.
2. **Keep the API small**:
   - A handful of musically meaningful parameters is better than dozens of low-level knobs.
3. **Keep the names coherent**:
   - Follow the Horizon naming feel (width, air, dirt, tilt, anchor, etc.).
   - Avoid generic names like `Processor`, `Filter1`, `Block2`.

If new projects live in separate repos, feel free to copy this `AGENTS.md` and adapt the
“Scope & Intent” and “Project Layout” sections.

---

## 9. How Agents Should Behave Socially (Meta)

If you are an AI or automated assistant proposing changes:

- Prefer **patch-sized suggestions** over large rewrites.
- Explain non-trivial changes in human language: what you changed, and why it matters musically.
- When asked for help, default to:
  - showing code that compiles in this environment,
  - preserving existing patterns,
  - and offering commentary that a student could learn from.

Remember: this code is not just for machines to run; it’s for people to think with.

---
