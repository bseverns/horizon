#pragma once

#include <stdint.h>

// Compatibility aliases for newer PJRC Audio sources built against the
// PlatformIO Teensy 4.18.0 core. The SAI FIFO watermark field is named
// I2S_TCR1_RFW in this core even when configuring transmit, while newer
// Audio code asks for the clearer I2S_TCR1_TFW name.
#ifndef I2S_TCR1_TFW
#define I2S_TCR1_TFW(n) ((uint32_t)(n) & 0x1f)
#endif

// Some Audio Git revisions enable the SAI transmit FIFO error interrupt, but
// the PlatformIO core only publishes the DMA request bits for Teensy 4.x.
// This bit matches the FIFO error interrupt enable position used by PJRC's
// I2S register map.
#ifndef I2S_TCSR_FEIE
#define I2S_TCSR_FEIE ((uint32_t)0x00000400)
#endif
