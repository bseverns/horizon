#pragma once

// Lint-only glue for hosts that don't have the Teensy core present.
// When clangd or a non-Teensy toolchain scans the tree, symbols like
// __disable_irq, NVIC_SET_PENDING, and Serial aren't visible because the
// PJRC headers live under PlatformIO's install dir. Rather than spam the
// repo with generated dependencies, we slip in these no-op shims so IDEs
// stay quiet while real firmware builds continue to use the genuine
// definitions from the Teensy core.

// Only turn the stubs on when we're *not* compiling for the actual
// Teensy toolchain.
#if !defined(__arm__) && !defined(TEENSYDUINO)

#include <cmath>
#include <cstdint>

using std::cos;

#ifndef __disable_irq
inline void __disable_irq() {}
#endif

#ifndef __enable_irq
inline void __enable_irq() {}
#endif

#ifndef F_CPU_ACTUAL
// Pick a sane default so math that depends on the clock can still
// evaluate in editors; the real value comes from the board config.
#define F_CPU_ACTUAL 600000000UL
#endif

#ifndef IRQ_SOFTWARE
#define IRQ_SOFTWARE 0
#endif

#ifndef NVIC_SET_PENDING
inline void NVIC_SET_PENDING(int) {}
#endif

// Super-light Serial facsimile so Audio library logging hooks type-check.
class HorizonLintSerial {
public:
    template <typename T>
    void print(const T &) {}

    template <typename T>
    void println(const T &) {}

    void begin(...) {}
};

#ifndef Serial
static HorizonLintSerial Serial;
#endif

#endif // host-only lint stubs
