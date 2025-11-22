// Studio note: parameter hand-hold. This one-pole glide keeps knobs from biting—rapid
// automation turns into gentle bends so the ear hears performance, not zipper noise.
#include "ParamSmoother.h"

static inline float clampf_ps(float x, float lo, float hi) {
  return (x < lo) ? lo : (x > hi ? hi : x);
}

void ParamSmoother::setSmoothing(float alpha) {
  _alpha = clampf_ps(alpha, 0.0f, 1.0f);
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
