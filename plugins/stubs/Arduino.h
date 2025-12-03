#pragma once
// Host-friendly stub for Arduino-style includes so JUCE builds can reuse the
// Teensy-oriented DSP code without pulling the full Arduino core. We only
// define the handful of types and helpers Horizon relies on.

#include <cstdint>
#include <cmath>

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef constrain
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#endif
