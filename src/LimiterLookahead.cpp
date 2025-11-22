// Studio note: safety ceiling with a musician’s handshake. We chase peaks fast so
// transients survive, then relax with a slow release so the gain ride feels like
// a mix engineer leaning on the fader instead of a brick wall clamp.
#include "LimiterLookahead.h"
#include <math.h>

static inline float clampf_lim(float x, float lo, float hi) {
  return (x < lo) ? lo : (x > hi ? hi : x);
}

static constexpr float kFsLim = 44100.0f;

float LimiterLookahead::dbToLin(float dB) {
  return powf(10.0f, dB / 20.0f);
}

LimiterLookahead::LimiterLookahead()
  : _ceiling(dbToLin(-1.0f)), _gain(1.0f), _releaseCoeff(0.0f) {
  // ~100 ms release
  float releaseSec = 0.1f;
  _releaseCoeff = expf(-1.0f / (releaseSec * kFsLim));
}

void LimiterLookahead::setCeilingDb(float dB) {
  dB = clampf_lim(dB, -18.0f, 0.0f);
  _ceiling = dbToLin(dB);
}

void LimiterLookahead::reset() {
  _gain = 1.0f;
}

void LimiterLookahead::processStereo(float& l, float& r) {
  float peak = fabsf(l);
  float ar = fabsf(r);
  if (ar > peak) peak = ar;

  float targetGain = 1.0f;
  if (peak > _ceiling && peak > 0.0f) {
    targetGain = _ceiling / peak;
  }

  // Fast attack (no lookahead yet), smooth release.
  if (targetGain < _gain) {
    _gain = targetGain;
  } else {
    _gain = _gain + (targetGain - _gain) * (1.0f - _releaseCoeff);
  }

  l *= _gain;
  r *= _gain;
}
