#pragma once

// Minimal Teensy 4 overlay: pass through to PJRC's AudioStream but
// ensure a few helper macros exist when the core forgets to publish
// them. Keep this lightweight so we stay aligned with upstream.

#include_next <AudioStream.h>

// If the real Teensy core isn't on the include path, make a minimal
// fallback so dependent libraries (like the PJRC Audio toolkit) still
// find AudioStream symbols during host-only or misconfigured builds.
// The goal is to keep compilation moving; real firmware should always
// rely on the upstream header.
#ifndef AudioStream_h
#define AudioStream_h

#include <stdint.h>

#ifndef AUDIO_BLOCK_SAMPLES
#define AUDIO_BLOCK_SAMPLES 128
#endif

#ifdef __cplusplus
typedef struct audio_block_struct {
    uint8_t ref_count;
    uint8_t reserved1;
    uint16_t memory_pool_index;
    int16_t data[AUDIO_BLOCK_SAMPLES];
} audio_block_t;

class AudioStream {
  public:
    AudioStream(unsigned char ninput, audio_block_t **iqueue) : num_inputs(ninput), inputQueue(iqueue) {}
    virtual void update() = 0;

  protected:
    static audio_block_t *allocate(void) { return nullptr; }
    static void release(audio_block_t *) {}
    audio_block_t *receiveReadOnly(uint32_t) { return nullptr; }
    audio_block_t *receiveWritable(uint32_t) { return nullptr; }
    void transmit(audio_block_t *, uint32_t = 0) {}

    unsigned char num_inputs;
    audio_block_t **inputQueue;
};
#endif  // __cplusplus

#endif // AudioStream_h

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

// Recent Teensy cores publish AUDIO_SAMPLE_RATE_EXACT, but host-only or older
// builds sometimes forget. Define a fallback that matches the Audio library's
// expected 44.1 kHz rate so dependent headers compile cleanly.
#ifndef AUDIO_SAMPLE_RATE_EXACT
#define AUDIO_SAMPLE_RATE_EXACT 44117.64706f
#endif
