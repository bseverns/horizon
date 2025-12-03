#pragma once

// Lightweight host-only Arduino shim so DSP blocks compile under PlatformIO's
// native runner. Keep it tiny: just the pieces needed by the DSP code, without
// macro landmines that step on the STL.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

inline unsigned long millis() { return 0; }
inline unsigned long micros() { return 0; }

// Arduino normally injects min/max macros, but those collide with <chrono> and
// other STL headers when building the host bench. Stick to inline helpers so
// unqualified calls still work without redefining language keywords.
template <typename T>
inline constexpr const T& min(const T& a, const T& b) {
    return (a < b) ? a : b;
}

template <typename T>
inline constexpr const T& max(const T& a, const T& b) {
    return (a > b) ? a : b;
}

#ifndef constrain
template <typename T>
inline T constrain(T x, T lo, T hi) {
    return (x < lo) ? lo : (x > hi ? hi : x);
}
#endif

// Arduino-ish types
using byte = uint8_t;
using word = uint16_t;

