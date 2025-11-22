// Studio note: tanh drive that behaves like a friendly tape machine. We normalize the
// curve so added dirt thickens harmonics without flattening dynamics—the math keeps
// the vibe crunchy yet polite.
#include "SoftSaturation.h"
#include <math.h>

static inline float clampf_ss(float x, float lo, float hi) {
  return (x < lo) ? lo : (x > hi ? hi : x);
}

SoftSaturation::SoftSaturation()
  : _amount(0.0f),
    _drive(1.0f),
    _invTanhDrive(1.0f) {}

void SoftSaturation::setAmount(float amt) {
  _amount = clampf_ss(amt, 0.0f, 1.0f);
  if (_amount <= 0.0001f) {
    _drive = 1.0f;
    _invTanhDrive = 1.0f;
    return;
  }
  // Map 0..1 to a musically useful drive range.
  _drive = 1.0f + 9.0f * _amount;
  float t = tanhf(_drive);
  if (t != 0.0f) {
    _invTanhDrive = 1.0f / t;
  } else {
    _invTanhDrive = 1.0f;
  }
}

float SoftSaturation::processSample(float x) const {
  if (_amount <= 0.0001f) {
    return x;
  }
  float y = tanhf(x * _drive) * _invTanhDrive;
  return y;
}

void SoftSaturation::processStereo(float& l, float& r) const {
  l = processSample(l);
  r = processSample(r);
}
