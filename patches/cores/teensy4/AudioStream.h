#pragma once

// Minimal Teensy 4 overlay: pass through to PJRC's AudioStream but
// ensure a few helper macros exist when the core forgets to publish
// them. Keep this lightweight so we stay aligned with upstream.
#if defined(__arm__)
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
#endif // __arm__

#include_next <AudioStream.h>
