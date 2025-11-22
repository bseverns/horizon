# Horizon Hardware Notes

This document describes the **hardware expectations** of the Horizon DSP engine and the
boards it runs on.

The short version:

- Firmware is written as if it’s running on a **Teensy 4.x** with an **SGTL5000 codec**.
- The **development rig** is an actual Teensy 4.0/4.1 + Audio Shield + loose controls.
- The **final board** is a custom IMXRT1062 + SGTL5000 design that **mirrors the same signal
  and pin assignments**, so the firmware doesn’t need to know the difference.

If the nets and pins described here exist, Horizon should “just work” on any board that
looks like a Teensy 4.x from the toolchain’s point of view.

---

## 1. System Overview

Horizon expects three main hardware blocks:

1. **Core MCU & boot**
   - NXP **IMXRT1062** MCU (same silicon as Teensy 4.x).
   - External QSPI flash (e.g., Winbond) for code storage.
   - PJRC **MKL02** bootloader MCU, wired per PJRC reference designs.
   - 24 MHz system crystal and associated passive components.
   - 3.3 V regulator and decoupling for core, I/O, and analog domains.
   - USB connector + ESD + required resistors, if USB is exposed.

2. **Audio codec**
   - **SGTL5000** codec connected via:
     - I²S for audio data (44.1 kHz, 16-bit stereo).
     - I²C for control (register access).
   - Analog front end:
     - Line input (for stereo bus feed).
     - Headphone and/or line output.
     - Power supply and filtering as per datasheet.

3. **Control surface**
   - 5 analog potentiometers (width, dynamic width, transient sensitivity, dirt, mix).
   - 2 momentary buttons (bypass, safe).
   - 1 indicator LED (limiter activity).

The firmware uses **Teensy pin numbers** and the PJRC Audio library. The custom board should
wire the IMXRT1062 and SGTL5000 so that these logical pin numbers map to the expected nets.

---

## 2. MCU Core & Bootloader

The IMXRT1062 core should be brought up following PJRC’s minimal Teensy 4.x reference:

- IMXRT1062 BGA with:
  - QSPI flash wired to the standard pads (as on Teensy 4.1).
  - 24 MHz crystal and appropriate loading caps.
  - Power rails for core and I/O (typically from a single 3.3 V regulator, with adequate
    decoupling and bulk capacitance).
- MKL02 bootloader MCU with:
  - Reset and program/boot pins connected to the IMXRT1062 as documented by PJRC.
  - USB D+ / D− and 5 V handling so the board enumerates like a Teensy.

From the firmware’s perspective, this board is simply “a Teensy 4.x without headers.”

---

## 3. SGTL5000 Audio Codec

The SGTL5000 codec is controlled and clocked as in the Teensy Audio Shield design. Horizon
assumes the following **logical Teensy pin assignments**:

### 3.1 Digital connections

| Function         | Teensy pin (logical) | Notes / Connects To                     |
|------------------|----------------------|-----------------------------------------|
| I²C SDA          | 18                   | SGTL5000 SDA                            |
| I²C SCL          | 19                   | SGTL5000 SCL                            |
| I²S MCLK         | 23 (A9)             | SGTL5000 MCLK                           |
| I²S LRCLK        | 20 (A6)             | SGTL5000 LRCLK                          |
| I²S BCLK         | 21 (A7)             | SGTL5000 BCLK                           |
| I²S TX (OUT1A)   | 7                    | SGTL5000 DIN (codec audio in)           |
| I²S RX (IN1)     | 8                    | SGTL5000 DOUT (codec audio out)         |

These pins should be routed from the IMXRT1062 pads that correspond to the Teensy 4.x pin
numbers shown above.

The firmware uses:

- `AudioInputI2S` + `AudioOutputI2S` for audio I/O.
- `AudioControlSGTL5000` for codec configuration (volume, filters, etc.).

### 3.2 Analog connections

Follow the SGTL5000 datasheet and/or PJRC Audio Shield reference for:

- AVDD / DVDD / HPVDD rails and their filtering.
- Line input coupling capacitors.
- Headphone output jack and any required output filtering.
- Line out, if present.

Grounding and layout for the codec should be treated as audio-grade:
- Keep analog paths short.
- Keep digital clocks and switching regulators away from the codec input nodes.
- Use a solid ground reference with minimal loop area between codec and input/output jacks.

---

## 4. Control Surface

Horizon’s current firmware assumes **5 pots, 2 buttons, and 1 LED** on specific Teensy pins.

### 4.1 Potentiometers

Each pot is wired:

- One side → 3.3 V
- Opposite side → GND
- Wiper → MCU analog pin (Teensy pin below)
- Optional per-pot RC: ~10 kΩ series from wiper to MCU pin, 100 nF to GND near the MCU, to
  tame noise.

| Function              | Teensy analog | Teensy digital | Notes             |
|-----------------------|---------------|----------------|-------------------|
| Width                 | A0            | 14             | Stereo width      |
| Dynamic width amount  | A2            | 16             | DynWidth mix      |
| Transient sensitivity | A3            | 17             | TransientDetector |
| Dirt amount           | A8            | 22             | SoftSaturation    |
| Wet/dry mix           | A10           | 24             | Horizon mix       |

The firmware reads these via `analogRead()` and maps them into:

- `setWidth(0.0 .. 1.0)`
- `setDynWidth(0.0 .. 1.0)`
- `setTransientSens(0.0 .. 1.0)`
- `setDirt(0.0 .. 1.0)` (often squared for finer low-end control)
- `setMix(0.0 .. 1.0)`

**Important:** Avoid sharing these analog pins with any other time-critical or noisy
functions. Treat them as dedicated control inputs.

### 4.2 Buttons

Buttons are simple momentary switches wired:

- One side → Teensy digital pin (with `INPUT_PULLUP` in firmware).
- Other side → GND.

| Function      | Teensy pin | Behavior                             |
|---------------|------------|--------------------------------------|
| Bypass button | 0          | Toggles wet/dry mix to 0 and back    |
| Safe button   | 1          | Clamps some parameters to safe range |

Debounce is handled in firmware using edge detection and simple logic.

### 4.3 Limiter LED

The limiter LED indicates when the limiter is actively reducing gain:

- MCU pin → series resistor (e.g. 330 Ω–1 kΩ) → LED → GND.

| Function   | Teensy pin | Behavior                                          |
|------------|------------|---------------------------------------------------|
| LIMIT LED  | 9          | On when limiter gain < ~0.7 (about 3 dB GR)      |

Firmware drives this based on `horizon.getLimiterGain()`.

---

## 5. USB & External Connectivity

The board **may expose USB** as:

- Audio device (USB_AUDIO build flag).
- MIDI / Serial as needed.

At minimum:

- USB D+ / D− routed from IMXRT1062 (and/or MKL02, per PJRC design) to a USB connector.
- 5 V handling and protection as recommended for Teensy-like boards.
- ESD protection on USB data lines if the connector is panel-mounted / user-facing.

Audio I/O should be available as:

- **Stereo line input** (for feed from mixer or interface).
- **Headphone or line output** (post-Horizon signal).

---

## 6. Layout Guidelines (Recommendations)

To keep Horizon quiet and stable:

- **Separate “noisy” and “quiet” zones**
  - Keep switching regulators, digital clocks, and fast edges physically away from SGTL5000
    inputs and analog traces.
  - Route I²S and I²C as short, tight pairs where possible.

- **Grounding**
  - Use a single, continuous ground plane where you can.
  - Avoid long, skinny ground traces under the codec and input/output connectors.
  - If you introduce “AGND” vs “DGND”, tie them at a single, well-chosen point.

- **Decoupling**
  - Place decoupling capacitors close to the IMXRT1062 and SGTL5000 power pins.
  - Add bulk capacitance near the main 3.3 V regulator and near the audio section.

- **Control lines**
  - Pot wiper traces can be relatively slow but should avoid running in parallel with
    high-current or high-frequency lines.
  - Buttons and LED traces are forgiving, but keep them away from sensitive analog nodes.

---

## 7. Bring-Up Checklist

A rough order of operations for first power-on:

1. **Power & USB**
   - Verify 3.3 V rail and current draw with no MCU programmed.
   - Connect over USB, verify the board enumerates using Teensy Loader / Teensyduino.
   - Flash a minimal “blink” or `Serial.print()` sketch.

2. **Core & Audio**
   - Flash a minimal Teensy Audio test (e.g., passthrough from I2S input to I2S output).
   - Confirm SGTL5000 comes up and audio passes cleanly.

3. **Horizon firmware**
   - Flash Horizon in “scope” build (e.g., `scope_teensy41` environment) to check:
     - Width / transient / limiter telemetry via USB Serial.
     - Codec configuration and signal routing.

4. **Controls**
   - Verify each pot moves the expected parameter:
     - width, dyn width, transient sensitivity, dirt, mix.
   - Check bypass button toggles between dry and processed signal.
   - Check safe button clamps behavior as designed.
   - Confirm limiter LED flickers appropriately under hot input.

Once these steps match the behavior of the known-good Teensy + Audio Shield dev stack,
the custom board is considered Horizon-compatible.

---

## 8. Future Extensions

This hardware spec is deliberately conservative. Potential future enhancements:

- Additional control inputs (encoders, switches, CV inputs) mapped to free Teensy pins.
- Dedicated LEDs or meters for width, level, or GR.
- MIDI over DIN or TRS, mapped to unused UART pins.
- Alternate codec configurations (e.g., balanced outputs, different input trims).

If you add new hardware features, update this document and keep the **Teensy pin table**
as the single source of truth for the firmware’s view of the world.
