// Studio note: this limiter is meant to feel like a human riding faders with a
// slight clairvoyance. We lean on a short delay line (1–8 ms) to see peaks
// coming, pre-emphasize the detector so bright hits register, modulate release
// based on transient activity, and crossfade bypass moves so nothing clicks.
#include "LimiterLookahead.h"
#include <Audio.h>
#include <math.h>

static inline float clampf_lim(float x, float lo, float hi) {
  return (x < lo) ? lo : (x > hi ? hi : x);
}

static constexpr float kSampleRate = AUDIO_SAMPLE_RATE_EXACT;
static constexpr float kMsToSec = 0.001f;

float LimiterLookahead::dbToLin(float dB) {
  return powf(10.0f, dB / 20.0f);
}

LimiterLookahead::LimiterLookahead()
  : _writeIdx(0),
    _lookaheadSamples(256),
    _mix(1.0f),
    _ceilingDb(-1.0f),
    _ceilingLin(dbToLin(-1.0f)),
    _releaseMs(80.0f),
    _releaseCoeff(0.0f),
    _env(1.0f),
    _lastTransientAvg(0.0f),
    _linkMode(LinkMode::Linked),
    _clipFlag(false),
    _bypassTarget(false),
    _bypassMix(0.0f),
    _xfadeSamples(0),
    _xfadeCountdown(0),
    _blockPos(0),
    _blockPeakIn(0.0f),
    _blockPeakOut(0.0f),
    _blockTransientSum(0.0f),
    _lastTelemetry{0.0f, 0.0f, 0.0f} {}

void LimiterLookahead::setup() {
  primeDelayLine();
  _lookaheadSamples = 256;
  _mix = 1.0f;
  _ceilingDb = -1.0f;
  _ceilingLin = dbToLin(_ceilingDb);
  _releaseMs = 80.0f;
  _env = 1.0f;
  _lastTransientAvg = 0.0f;
  _linkMode = LinkMode::Linked;
  _clipFlag = false;
  _bypassTarget = false;
  _bypassMix = 0.0f;
  _xfadeSamples = static_cast<int>(kSampleRate * 0.005f + 0.5f);
  if (_xfadeSamples < 1) _xfadeSamples = 1;
  _xfadeCountdown = 0;
  _blockPos = 0;
  _blockPeakIn = _blockPeakOut = 0.0f;
  _blockTransientSum = 0.0f;
  _lastTelemetry = {0.0f, 0.0f, 0.0f};

  _detectorTiltL.setTiltDbPerOct(0.0f);
  _detectorTiltR.setTiltDbPerOct(0.0f);
  _detectorTiltL.reset();
  _detectorTiltR.reset();
  _transientDetector.setSensitivity(0.6f);
  _transientDetector.reset();
  _safetyClip.setAmount(0.05f);

  updateReleaseCoeff();
}

void LimiterLookahead::primeDelayLine() {
  for (int i = 0; i < kBufferSize; ++i) {
    _bufferL[i] = 0.0f;
    _bufferR[i] = 0.0f;
  }
  _writeIdx = 0;
}

void LimiterLookahead::setCeilingDb(float dB) {
  _ceilingDb = clampf_lim(dB, -12.0f, -0.1f);
  _ceilingLin = dbToLin(_ceilingDb);
}

void LimiterLookahead::setReleaseMs(float ms) {
  _releaseMs = clampf_lim(ms, 20.0f, 200.0f);
  updateReleaseCoeff();
}

void LimiterLookahead::setLookaheadMs(float ms) {
  float clamped = clampf_lim(ms, 1.0f, 8.0f);
  int samples = static_cast<int>(clamped * kMsToSec * kSampleRate + 0.5f);
  if (samples < 1) samples = 1;
  if (samples >= kBufferSize) samples = kBufferSize - 1;
  _lookaheadSamples = samples;
}

void LimiterLookahead::setDetectorTiltDbPerOct(float db) {
  float tilt = clampf_lim(db, -3.0f, 3.0f);
  _detectorTiltL.setTiltDbPerOct(tilt);
  _detectorTiltR.setTiltDbPerOct(tilt);
}

void LimiterLookahead::setLinkMode(LinkMode mode) {
  _linkMode = mode;
}

void LimiterLookahead::setMix(float mix01) {
  _mix = clampf_lim(mix01, 0.0f, 1.0f);
}

void LimiterLookahead::setBypass(bool on) {
  if (on != _bypassTarget) {
    _bypassTarget = on;
    _xfadeCountdown = _xfadeSamples;
  }
}

void LimiterLookahead::reset() {
  primeDelayLine();
  _env = 1.0f;
  _blockPos = 0;
  _blockPeakIn = _blockPeakOut = 0.0f;
  _blockTransientSum = 0.0f;
  _clipFlag = false;
  _bypassMix = _bypassTarget ? 1.0f : 0.0f;
  _xfadeCountdown = 0;
  _detectorTiltL.reset();
  _detectorTiltR.reset();
  _transientDetector.reset();
}

void LimiterLookahead::updateReleaseCoeff() {
  float fastMs = _releaseMs * 0.25f;
  if (fastMs < 10.0f) fastMs = 10.0f;
  float slowMs = _releaseMs;
  float t = 1.0f - clampf_lim(_lastTransientAvg, 0.0f, 1.0f);
  float releaseMs = fastMs + (slowMs - fastMs) * t;
  float releaseSec = releaseMs * kMsToSec;
  float alpha = 1.0f - expf(-1.0f / (releaseSec * kSampleRate));
  if (alpha < 0.0f) alpha = 0.0f;
  _releaseCoeff = alpha;
}

void LimiterLookahead::updateXfade() {
  if (_xfadeCountdown <= 0) return;
  float step = 1.0f / static_cast<float>(_xfadeSamples);
  if (_bypassTarget) {
    _bypassMix += step;
    if (_bypassMix > 1.0f) _bypassMix = 1.0f;
  } else {
    _bypassMix -= step;
    if (_bypassMix < 0.0f) _bypassMix = 0.0f;
  }
  --_xfadeCountdown;
}

void LimiterLookahead::finalizeBlock() {
  // Update telemetry once per audio block to keep things steady for Serial/UI.
  _lastTelemetry.env_db = 20.0f * log10f(_env + kEps);
  _lastTelemetry.peak_in = _blockPeakIn;
  _lastTelemetry.peak_out = _blockPeakOut;

  // Compute average transient for adaptive release.
  float avg = 0.0f;
  if (_blockPos > 0) {
    avg = _blockTransientSum / static_cast<float>(_blockPos);
  }
  _lastTransientAvg = avg;
  updateReleaseCoeff();

  _blockPos = 0;
  _blockPeakIn = 0.0f;
  _blockPeakOut = 0.0f;
  _blockTransientSum = 0.0f;
}

float LimiterLookahead::getGRdB() const {
  return 20.0f * log10f(_env + kEps);
}

bool LimiterLookahead::getClipFlagAndClear() {
  bool hit = _clipFlag;
  _clipFlag = false;
  return hit;
}

void LimiterLookahead::processStereo(float& l, float& r) {
  // Start-of-block bookkeeping
  if (_blockPos == 0) {
    updateReleaseCoeff();
  }

  // Detector feed: tilt, stereo link, transient score
  float detL = _detectorTiltL.processSample(l);
  float detR = _detectorTiltR.processSample(r);
  float level;
  if (_linkMode == LinkMode::MidSide) {
    float m = 0.5f * (detL + detR);
    float s = 0.5f * (detL - detR);
    level = fabsf(m);
    float absS = fabsf(s);
    if (absS > level) level = absS;
  } else {
    level = fabsf(detL);
    float absR = fabsf(detR);
    if (absR > level) level = absR;
  }
  if (level < kEps) level = kEps;

  float transient = _transientDetector.processSample(level);
  _blockTransientSum += transient;

  // Lookahead delay lines for dry + wet alignment
  _bufferL[_writeIdx] = l;
  _bufferR[_writeIdx] = r;
  int readIdx = _writeIdx - _lookaheadSamples;
  if (readIdx < 0) readIdx += kBufferSize;
  float dryL = _bufferL[readIdx];
  float dryR = _bufferR[readIdx];
  _writeIdx = (_writeIdx + 1) % kBufferSize;

  // Required gain and envelope
  float gReq = _ceilingLin / level;
  if (gReq > 1.0f) gReq = 1.0f;
  float relaxing = _env + _releaseCoeff * (1.0f - _env);
  _env = (gReq < relaxing) ? gReq : relaxing;
  if (_env < 0.0f) _env = 0.0f;

  float wetL = dryL * _env;
  float wetR = dryR * _env;

  _safetyClip.processStereo(wetL, wetR);

  if (wetL > _ceilingLin) { wetL = _ceilingLin; _clipFlag = true; }
  if (wetL < -_ceilingLin) { wetL = -_ceilingLin; _clipFlag = true; }
  if (wetR > _ceilingLin) { wetR = _ceilingLin; _clipFlag = true; }
  if (wetR < -_ceilingLin) { wetR = -_ceilingLin; _clipFlag = true; }

  float mixedL = dryL + _mix * (wetL - dryL);
  float mixedR = dryR + _mix * (wetR - dryR);

  updateXfade();
  float outL = mixedL * (1.0f - _bypassMix) + dryL * _bypassMix;
  float outR = mixedR * (1.0f - _bypassMix) + dryR * _bypassMix;

  _blockPeakIn = fmaxf(_blockPeakIn, fmaxf(fabsf(l), fabsf(r)));
  _blockPeakOut = fmaxf(_blockPeakOut, fmaxf(fabsf(outL), fabsf(outR)));

  l = outL;
  r = outR;

  ++_blockPos;
  if (_blockPos >= AUDIO_BLOCK_SAMPLES) {
    finalizeBlock();
  }
}

uint8_t mapGRtoLeds(float gr_db) {
  // Thresholds: −1, −2, −3, −4, −6, −8, −10, −12 dB
  static const float thresholds[8] = {-1.0f, -2.0f, -3.0f, -4.0f, -6.0f, -8.0f, -10.0f, -12.0f};
  uint8_t count = 0;
  for (int i = 0; i < 8; ++i) {
    if (gr_db <= thresholds[i]) {
      count = i + 1;
    }
  }
  return count;
}
