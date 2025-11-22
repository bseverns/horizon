#pragma once

class LimiterLookahead {
public:
  LimiterLookahead();

  // Ceiling in dBFS (e.g. -1.0)
  void setCeilingDb(float dB);

  void reset();

  // In-place stereo sample processing, -1..1 domain.
  void processStereo(float& l, float& r);

  float getGain() const { return _gain; }

private:
  float _ceiling;
  float _gain;
  float _releaseCoeff;

  static float dbToLin(float dB);
};
