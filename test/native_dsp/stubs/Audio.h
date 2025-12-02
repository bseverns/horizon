#pragma once

// Bare-minimum Audio library shim for host-side testing.
// We only surface the constants used by the DSP code.

#include <cstdint>

// Teensy Audio defaults: 44.1 kHz sample rate, 128-sample blocks.
#define AUDIO_SAMPLE_RATE_EXACT 44100.0f
#define AUDIO_BLOCK_SAMPLES 128

// Audio library would normally pull in arm_math intrinsics; on host we just
// use <cmath> via Arduino shim, so nothing else to do here.

