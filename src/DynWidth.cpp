#include "DynWidth.h"
#include <math.h>

static inline float clampf_dw(float x, float lo, float hi) {
  return (x < lo) ? lo : (x > hi ? hi : x);
}

static constexpr float kFsDW = 44100.0f;
static constexpr float kTwoPiDW = 6.28318530717958647692f;

DynWidth::DynWidth()
  : _baseWidth(0.6f),
    _dynAmt(0.35f),
    _lowAnchorHz(100.0f),
    _lowSideState(0.0f),
    _lowAlpha(0.0f),
    _lastWidth(0.6f) {
  setLowAnchorHz(_lowAnchorHz);
}

void DynWidth::reset() {
  _lowSideState = 0.0f;
  _lastWidth = _baseWidth;
}

void DynWidth::setBaseWidth(float w) {
  _baseWidth = clampf_dw(w, 0.0f, 1.5f);
  _lastWidth = _baseWidth;
}

void DynWidth::setDynAmount(float a) {
  _dynAmt = clampf_dw(a, 0.0f, 1.0f);
}

void DynWidth::setLowAnchorHz(float hz) {
  _lowAnchorHz = clampf_dw(hz, 40.0f, 250.0f);
  float omega = kTwoPiDW * _lowAnchorHz / kFsDW;
  _lowAlpha = 1.0f - expf(-omega);
}

void DynWidth::processSample(float& mid, float& side, float transientActivity) {
  (void)mid; // currently mid is untouched here; kept for future shaping.
  float t = clampf_dw(transientActivity, 0.0f, 1.0f);

  // Split side into low and high around lowAnchor.
  _lowSideState += _lowAlpha * (side - _lowSideState);
  float low  = _lowSideState;
  float high = side - low;

  // Static + dynamic width:
  float widen  = _baseWidth + _dynAmt;                // widen in tails
  float narrow = _baseWidth * (1.0f - 0.9f*_dynAmt);  // narrow on hits
  if (widen  > 1.5f) widen  = 1.5f;
  if (narrow < 0.0f) narrow = 0.0f;

  float widthNow = narrow * t + widen * (1.0f - t);
  _lastWidth = widthNow;

  // Extra mono-izing for very low frequencies.
  float lowWidthScale = 0.25f;

  high *= widthNow;
  low  *= widthNow * lowWidthScale;

  side = low + high;
}
