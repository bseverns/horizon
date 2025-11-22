// Studio note: fast tilt around 1 kHz to lean mixes warm or airy. Tilt is set in
// dB/oct (-6..+6), mapped to a matched shelf pair so the pivot stays honest. Both
// shelves share one pole to keep phase kind—think changing the angle of a spotlight,
// not carving with a scalpel.
#include "TiltEQ.h"
#include <math.h>

static inline float clampf_tilt(float x, float lo, float hi) {
  return (x < lo) ? lo : (x > hi ? hi : x);
}

static constexpr float kFsTilt = 44100.0f;
static constexpr float kTwoPiTilt = 6.28318530717958647692f;
static constexpr float kPivotHzTilt = 1000.0f;

float TiltEQ::dbToLin(float dB) {
  return powf(10.0f, dB / 20.0f);
}

TiltEQ::TiltEQ()
  : _alpha(0.0f), _low(0.0f), _tiltLowGain(1.0f), _tiltHighGain(1.0f) {
  setTiltDbPerOct(0.0f);
}

void TiltEQ::setTiltDbPerOct(float dBPerOct) {
  dBPerOct = clampf_tilt(dBPerOct, -6.0f, 6.0f);
  // Map dB/oct roughly to a symmetric shelf pair around the pivot.
  // This isn't a physical dB/oct relationship, but it's musical and simple:
  float totalShelf = dBPerOct * 2.0f; // up to ~12 dB spread
  float half = 0.5f * totalShelf;
  _tiltLowGain  = dbToLin(-half);
  _tiltHighGain = dbToLin(+half);

  float omega = kTwoPiTilt * kPivotHzTilt / kFsTilt;
  _alpha = 1.0f - expf(-omega);
}

void TiltEQ::reset() {
  _low = 0.0f;
}

float TiltEQ::processSample(float x) {
  _low += _alpha * (x - _low);
  float low = _low;
  float high = x - low;
  return low * _tiltLowGain + high * _tiltHighGain;
}
