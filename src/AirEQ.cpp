// Studio note: one-pole split EQ that lets the top octave breathe without harshness.
// Frequency sweeps ~4–16 kHz and gain runs -6..+6 dB; we low-pass once, subtract to
// get the fizz, then scale the fizz. A single pole keeps the phase tame so boosts
// feel like opening a window instead of poking with a scalpel.
#include "AirEQ.h"
#include <math.h>

static inline float clampf_air(float x, float lo, float hi) {
  return (x < lo) ? lo : (x > hi ? hi : x);
}

static constexpr float kTwoPiAir = 6.28318530717958647692f;

static float onePoleAlpha(float freqHz, float sampleRate) {
  const float c = cosf(kTwoPiAir * freqHz / sampleRate);
  const float a = 2.0f - c;
  return 1.0f - (a - sqrtf(a * a - 1.0f));
}

float AirEQ::dbToLin(float dB) {
  return powf(10.0f, dB / 20.0f);
}

AirEQ::AirEQ()
  : _alpha(0.0f), _low(0.0f), _highGain(1.0f), _freqHz(10000.0f),
    _sampleRate(44100.0f) {
  setFreqAndGain(10000.0f, 0.0f);
}

void AirEQ::setSampleRate(float sampleRate) {
  if (sampleRate > 0.0f) {
    _sampleRate = sampleRate;
    _alpha = onePoleAlpha(_freqHz, _sampleRate);
  }
}

void AirEQ::setFreqAndGain(float freqHz, float gainDb) {
  freqHz = clampf_air(freqHz, 4000.0f, 16000.0f);
  _freqHz = freqHz;
  _alpha = onePoleAlpha(_freqHz, _sampleRate);
  _highGain = dbToLin(clampf_air(gainDb, -6.0f, 6.0f));
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
