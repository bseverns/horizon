#include "TransientDetector.h"
#include <math.h>

static inline float clampf_td(float x, float lo, float hi) {
  return (x < lo) ? lo : (x > hi ? hi : x);
}

static constexpr float kFsTD = 44100.0f;

TransientDetector::TransientDetector()
  : _env(0.0f),
    _sensitivity(0.5f),
    _attackCoeff(0.0f),
    _releaseCoeff(0.0f) {
  // 2 ms attack, 80 ms release as a starting point.
  float attackSec  = 0.002f;
  float releaseSec = 0.080f;
  _attackCoeff  = 1.0f - expf(-1.0f / (attackSec  * kFsTD));
  _releaseCoeff = 1.0f - expf(-1.0f / (releaseSec * kFsTD));
}

void TransientDetector::reset() {
  _env = 0.0f;
}

void TransientDetector::setSensitivity(float s) {
  _sensitivity = clampf_td(s, 0.0f, 1.0f);
}

float TransientDetector::processSample(float x) {
  float ax = fabsf(x);
  float coeff = (ax > _env) ? _attackCoeff : _releaseCoeff;
  _env += coeff * (ax - _env);

  // Map env + sensitivity to a 0..1 transient activity measure.
  float threshold = 0.05f + _sensitivity * 0.45f; // ~0.05 .. 0.5
  float activity = 0.0f;
  if (_env > threshold) {
    activity = (_env - threshold) / (1.0f - threshold);
  }
  if (activity < 0.0f) activity = 0.0f;
  if (activity > 1.0f) activity = 1.0f;
  return activity;
}
