#pragma once
#include <cstddef>

// See docs/block_notes.md for musician-facing notes on this block.

class AirEQ {
public:
  AirEQ();

  // freqHz: approx 4k..16k, gainDb: -12..+12 (clamped)
  void setFreqAndGain(float freqHz, float gainDb);

  void reset();

  float processSample(float x);

private:
  float _alpha;
  float _low;
  float _highGain;

  static float dbToLin(float dB);
};
