// Studio note: envelope follower tuned for “is this a hit?” Sensitivity 0..1 shifts
// the threshold (~0.05..0.5), while dual 2 ms / 80 ms attack-release makes the meter
// jump on stick attacks then chill quickly. That transient pulse drives width and dirt
// moves that vibe with the drummer instead of fighting them.
#include "TransientDetector.h"
#include <math.h>

static inline float clampf_td(float x, float lo, float hi) {
  return (x < lo) ? lo : (x > hi ? hi : x);
}

static constexpr float kAttackSec = 0.002f;
static constexpr float kReleaseSec = 0.080f;

TransientDetector::TransientDetector()
  : _env(0.0f),
    _sensitivity(0.5f),
    _attackCoeff(0.0f),
    _releaseCoeff(0.0f),
    _sampleRate(44100.0f) {
  setSampleRate(_sampleRate);
}

void TransientDetector::reset() {
  _env = 0.0f;
}

void TransientDetector::setSensitivity(float s) {
  _sensitivity = clampf_td(s, 0.0f, 1.0f);
}

void TransientDetector::setSampleRate(float sampleRate) {
  if (sampleRate <= 0.0f) {
    return;
  }
  _sampleRate = sampleRate;
  _attackCoeff  = 1.0f - expf(-1.0f / (kAttackSec  * _sampleRate));
  _releaseCoeff = 1.0f - expf(-1.0f / (kReleaseSec * _sampleRate));
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
