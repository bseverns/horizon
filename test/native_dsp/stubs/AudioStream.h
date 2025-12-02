#pragma once

// Host-only AudioStream shim. Just enough plumbing for AudioHorizon to run
// inside the native PlatformIO test bench without the Teensy runtime.

#include <cstdint>

#include "Audio.h"

// Forward-declared in the host driver to catch transmitted blocks.
extern void horizon_host_transmit(audio_block_t *block, uint32_t channel, class AudioStream *self);

class AudioStream {
  public:
    AudioStream(unsigned char numInputs, audio_block_t **inputQueue)
        : _numInputs(numInputs), _inputQueue(inputQueue) {}

    virtual ~AudioStream() = default;
    virtual void update() {}

    static audio_block_t *allocate() { return new audio_block_t(); }
    static void release(audio_block_t *block) { delete block; }

  protected:
    audio_block_t *receiveReadOnly(uint32_t channel) {
        if (!_inputQueue || channel >= _numInputs) return nullptr;
        audio_block_t *blk = _inputQueue[channel];
        _inputQueue[channel] = nullptr;
        return blk;
    }

    void transmit(audio_block_t *block, uint32_t channel = 0) {
        if (!block) return;
        if (horizon_host_transmit) {
            horizon_host_transmit(block, channel, this);
        }
    }

  private:
    unsigned char _numInputs;
    audio_block_t **_inputQueue;
};

