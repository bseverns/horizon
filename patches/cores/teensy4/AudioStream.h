#pragma once

// Minimal Teensy 4 overlay: pass through to PJRC's AudioStream but
// ensure a few helper macros exist when the core forgets to publish
// them. Keep this lightweight so we stay aligned with upstream.

#include_next <AudioStream.h>

// Older Teensy cores (or host-only tooling) sometimes omit these
// convenience macros. Guard them to avoid trampling the official
// definitions shipped with recent TeensyDuino releases, which already
// provide IRQ_SOFTWARE and NVIC_SET_PENDING.
#if !defined(TEENSYDUINO) || (defined(TEENSYDUINO) && TEENSYDUINO < 158)
#ifndef F_CPU_ACTUAL
#define F_CPU_ACTUAL F_CPU
#endif

#ifndef IRQ_SOFTWARE
// Software interrupt slot used by the Audio library on i.MX RT.
#define IRQ_SOFTWARE 71
#endif

#ifndef NVIC_SET_PENDING
#define NVIC_SET_PENDING(irq) NVIC_SetPendingIRQ(static_cast<IRQn_Type>(irq))
#endif
#endif // legacy or host fallback
