#pragma once

// See docs/block_notes.md for musician-facing notes on this block.

class TiltEQ {
public:
  TiltEQ();

  // dBPerOct roughly -6..+6, positive tilts brighten highs.
  void setTiltDbPerOct(float dBPerOct);
  void setSampleRate(float sampleRate);

  void reset();

  float processSample(float x);

private:
  float _alpha;
  float _low;
  float _tiltLowGain;
  float _tiltHighGain;
  float _sampleRate;

  static float dbToLin(float dB);
};
