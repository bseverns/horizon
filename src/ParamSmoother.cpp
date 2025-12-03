// Studio note: parameter hand-hold. Alpha 0..1 sets the one-pole glide (0 = instant,
// 1 = snail); rapid automation turns into gentle bends so the ear hears performance,
// not zipper noise. First call seeds the value so there’s no surprise jump.
#include "ParamSmoother.h"
#include <math.h>

static inline float clampf_ps(float x, float lo, float hi) {
  return (x < lo) ? lo : (x > hi ? hi : x);
}

void ParamSmoother::setSmoothing(float alpha) {
  _alpha = clampf_ps(alpha, 0.0f, 1.0f);
}

void ParamSmoother::setTimeConstantMs(float ms, double sampleRate, int samplesPerUpdate) {
  if (ms <= 0.0f || sampleRate <= 0.0 || samplesPerUpdate <= 0) {
    setSmoothing(1.0f);
    return;
  }

  float dtSec = static_cast<float>(samplesPerUpdate / sampleRate);
  float tauSec = ms * 0.001f;
  float alpha = 1.0f - expf(-dtSec / tauSec);
  setSmoothing(alpha);
}

void ParamSmoother::reset(float v) {
  _value = v;
  _initialized = true;
}

float ParamSmoother::process(float target) {
  if (!_initialized) {
    _value = target;
    _initialized = true;
    return _value;
  }
  _value += _alpha * (target - _value);
  return _value;
}
