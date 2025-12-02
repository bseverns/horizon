#pragma once

// Lightweight host-only Arduino shim so DSP blocks compile under PlatformIO's
// native runner. Keep it tiny: just the pieces needed by the DSP code.

#include <cstdint>
#include <cstddef>
#include <cmath>

inline unsigned long millis() { return 0; }
inline unsigned long micros() { return 0; }

// Common helpers found in Arduino.h
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef constrain
template <typename T>
inline T constrain(T x, T lo, T hi) {
    return (x < lo) ? lo : (x > hi ? hi : x);
}
#endif

// Arduino-ish types
using byte = uint8_t;
using word = uint16_t;

