// Studio note: one-pole split EQ that lets the top octave breathe without harshness.
// We low-pass once, subtract to get the fizz, then scale the fizz: the single pole
// keeps phase tame so boosts feel like opening a window instead of poking with a scalpel.
#include "AirEQ.h"
#include <math.h>

static inline float clampf_air(float x, float lo, float hi) {
  return (x < lo) ? lo : (x > hi ? hi : x);
}

static constexpr float kFsAir = 44100.0f;
static constexpr float kTwoPiAir = 6.28318530717958647692f;

float AirEQ::dbToLin(float dB) {
  return powf(10.0f, dB / 20.0f);
}

AirEQ::AirEQ() : _alpha(0.0f), _low(0.0f), _highGain(1.0f) {
  setFreqAndGain(10000.0f, 0.0f);
}

void AirEQ::setFreqAndGain(float freqHz, float gainDb) {
  freqHz = clampf_air(freqHz, 4000.0f, 16000.0f);
  float omega = kTwoPiAir * freqHz / kFsAir;
  _alpha = 1.0f - expf(-omega);
  _highGain = dbToLin(clampf_air(gainDb, -12.0f, 12.0f));
}

void AirEQ::reset() {
  _low = 0.0f;
}

float AirEQ::processSample(float x) {
  // One-pole low-pass split
  _low += _alpha * (x - _low);
  float low = _low;
  float high = x - low;
  return low + high * _highGain;
}
