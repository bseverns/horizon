#pragma once

// See docs/block_notes.md for musician-facing notes on this block.

class DynWidth {
public:
  DynWidth();

  void reset();

  // base width 0..1 (~0.6 default)
  void setBaseWidth(float w);

  // dynamic amount 0..1 (0 = static width, 1 = strong transients)
  void setDynAmount(float a);

  // Low anchor frequency in Hz (roughly 40..200).
  void setLowAnchorHz(float hz);

  void setSampleRate(float sampleRate);

  // mid and side are in-place; transientActivity 0..1.
  void processSample(float& mid, float& side, float transientActivity);

  float getLastWidth() const { return _lastWidth; }

private:
  float _baseWidth;
  float _dynAmt;
  float _lowAnchorHz;
  float _lowSideState;
  float _lowAlpha;

  float _sampleRate;

  float _lastWidth;
};
