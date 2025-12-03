// HostHorizonProcessor lifts the AudioHorizon signal path into a plain C++
// wrapper for DAWs, tests, or any non-Teensy host. Same DSP toys, but fed by
// raw float buffers instead of PJRC audio blocks.
#include "HostHorizonProcessor.h"

namespace {
constexpr float kLimiterCeilingMinDb = -12.0f;
constexpr float kLimiterCeilingMaxDb = -0.1f;
constexpr float kOutputClamp = 1.0f;
}

HostHorizonProcessor::HostHorizonProcessor()
  : _widthTarget(0.6f),
    _dynWidthTarget(0.35f),
    _transientSensTarget(0.5f),
    _midTiltTarget(0.0f),
    _sideAirFreqTarget(10000.0f),
    _sideAirGainTarget(2.0f),
    _lowAnchorHzTarget(100.0f),
    _dirtTarget(0.1f),
    _ceilingDbTarget(-1.0f),
    _limiterReleaseTargetMs(80.0f),
    _limiterLookaheadTargetMs(5.8f),
    _limiterTiltTarget(0.0f),
    _limiterMixTarget(0.7f),
    _limiterLinkModeTarget(LimiterLookahead::LinkMode::Linked),
    _limiterBypassTarget(false),
    _mixTarget(0.6f),
    _outTrimDbTarget(0.0f),
    _telemetryWidth(_widthTarget),
    _telemetryTransient(0.0f),
    _telemetryLimiterGain(1.0f),
    _limiterTelemetry{0.0f, 0.0f, 0.0f} {
  _widthSm.setSmoothing(0.08f);
  _dynWidthSm.setSmoothing(0.08f);
  _transientSm.setSmoothing(0.08f);
  _midTiltSm.setSmoothing(0.08f);
  _sideAirFreqSm.setSmoothing(0.08f);
  _sideAirGainSm.setSmoothing(0.08f);
  _lowAnchorSm.setSmoothing(0.08f);
  _dirtSm.setSmoothing(0.08f);
  _ceilingSm.setSmoothing(0.08f);
  _limiterReleaseSm.setSmoothing(0.08f);
  _limiterLookaheadSm.setSmoothing(0.08f);
  _limiterTiltSm.setSmoothing(0.08f);
  _limiterMixSm.setSmoothing(0.08f);
  _mixSm.setSmoothing(0.1f);
  _outTrimSm.setSmoothing(0.1f);

  _dynWidth.setBaseWidth(_widthTarget);
  _dynWidth.setDynAmount(_dynWidthTarget);
  _dynWidth.setLowAnchorHz(_lowAnchorHzTarget);
  _detector.setSensitivity(_transientSensTarget);
  _midTilt.setTiltDbPerOct(_midTiltTarget);
  _sideAir.setFreqAndGain(_sideAirFreqTarget, _sideAirGainTarget);
  _softSat.setAmount(_dirtTarget);
  _limiter.setup();
  _limiter.setCeilingDb(_ceilingDbTarget);
  _limiter.setReleaseMs(_limiterReleaseTargetMs);
  _limiter.setLookaheadMs(_limiterLookaheadTargetMs);
  _limiter.setDetectorTiltDbPerOct(_limiterTiltTarget);
  _limiter.setLinkMode(_limiterLinkModeTarget);
  _limiter.setMix(_limiterMixTarget);
}

float HostHorizonProcessor::clampf(float x, float lo, float hi) const {
  return std::max(lo, std::min(x, hi));
}

void HostHorizonProcessor::setWidth(float w) {
  _widthTarget = clampf(w, 0.0f, 1.0f);
}

void HostHorizonProcessor::setDynWidth(float a) {
  _dynWidthTarget = clampf(a, 0.0f, 1.0f);
}

void HostHorizonProcessor::setTransientSens(float s) {
  _transientSensTarget = clampf(s, 0.0f, 1.0f);
}

void HostHorizonProcessor::setMidTilt(float dBPerOct) {
  _midTiltTarget = clampf(dBPerOct, -6.0f, 6.0f);
}

void HostHorizonProcessor::setSideAir(float freqHz, float gainDb) {
  _sideAirFreqTarget = clampf(freqHz, 4000.0f, 16000.0f);
  _sideAirGainTarget = clampf(gainDb, -6.0f, 6.0f);
}

void HostHorizonProcessor::setLowAnchor(float hz) {
  _lowAnchorHzTarget = clampf(hz, 40.0f, 250.0f);
}

void HostHorizonProcessor::setDirt(float amt) {
  _dirtTarget = clampf(amt, 0.0f, 1.0f);
}

void HostHorizonProcessor::setCeiling(float dB) {
  _ceilingDbTarget = clampf(dB, kLimiterCeilingMinDb, kLimiterCeilingMaxDb);
}

void HostHorizonProcessor::setLimiterReleaseMs(float ms) {
  _limiterReleaseTargetMs = clampf(ms, 20.0f, 200.0f);
}

void HostHorizonProcessor::setLimiterLookaheadMs(float ms) {
  _limiterLookaheadTargetMs = clampf(ms, 1.0f, 8.0f);
}

void HostHorizonProcessor::setLimiterDetectorTilt(float dBPerOct) {
  _limiterTiltTarget = clampf(dBPerOct, -3.0f, 3.0f);
}

void HostHorizonProcessor::setLimiterLinkMode(LimiterLookahead::LinkMode mode) {
  _limiterLinkModeTarget = mode;
}

void HostHorizonProcessor::setLimiterMix(float m) {
  _limiterMixTarget = clampf(m, 0.0f, 1.0f);
}

void HostHorizonProcessor::setLimiterBypass(bool on) {
  _limiterBypassTarget = on;
}

void HostHorizonProcessor::setMix(float m) {
  _mixTarget = clampf(m, 0.0f, 1.0f);
  _limiterMixTarget = _mixTarget;
}

void HostHorizonProcessor::setOutputTrim(float dB) {
  _outTrimDbTarget = clampf(dB, -12.0f, 6.0f);
}

void HostHorizonProcessor::processBlock(float* inL,
                                        float* inR,
                                        float* outL,
                                        float* outR,
                                        int numFrames,
                                        double sampleRate) {
  (void)sampleRate;

  if (!inL || !inR || !outL || !outR || numFrames <= 0) {
    return;
  }

  float width      = _widthSm.process(_widthTarget);
  float dynAmt     = _dynWidthSm.process(_dynWidthTarget);
  float sens       = _transientSm.process(_transientSensTarget);
  float midTiltDb  = _midTiltSm.process(_midTiltTarget);
  float airFreq    = _sideAirFreqSm.process(_sideAirFreqTarget);
  float airGainDb  = _sideAirGainSm.process(_sideAirGainTarget);
  float lowAnchor  = _lowAnchorSm.process(_lowAnchorHzTarget);
  float dirtAmt    = _dirtSm.process(_dirtTarget);
  float ceilingDb  = _ceilingSm.process(_ceilingDbTarget);
  float limRelease = _limiterReleaseSm.process(_limiterReleaseTargetMs);
  float limLook    = _limiterLookaheadSm.process(_limiterLookaheadTargetMs);
  float limTilt    = _limiterTiltSm.process(_limiterTiltTarget);
  float limMix     = _limiterMixSm.process(_limiterMixTarget);
  float outTrimDb  = _outTrimSm.process(_outTrimDbTarget);
  float outTrimLin = std::pow(10.0f, 0.05f * outTrimDb);

  _dynWidth.setBaseWidth(width);
  _dynWidth.setDynAmount(dynAmt);
  _dynWidth.setLowAnchorHz(lowAnchor);
  _detector.setSensitivity(sens);
  _midTilt.setTiltDbPerOct(midTiltDb);
  _sideAir.setFreqAndGain(airFreq, airGainDb);
  _softSat.setAmount(dirtAmt);
  _limiter.setCeilingDb(ceilingDb);
  _limiter.setReleaseMs(limRelease);
  _limiter.setLookaheadMs(limLook);
  _limiter.setDetectorTiltDbPerOct(limTilt);
  _limiter.setLinkMode(_limiterLinkModeTarget);
  _limiter.setMix(limMix);
  _limiter.setBypass(_limiterBypassTarget);

  for (int i = 0; i < numFrames; ++i) {
    float l = inL[i];
    float r = inR[i];

    float m, s;
    _ms.encode(l, r, m, s);

    m = _midTilt.processSample(m);
    s = _sideAir.processSample(s);

    float detectorIn = 0.5f * (std::fabs(m) + std::fabs(s));
    float activity = _detector.processSample(detectorIn);
    _telemetryTransient = activity;

    _dynWidth.processSample(m, s, activity);
    _telemetryWidth = _dynWidth.getLastWidth();

    float wetL, wetR;
    _ms.decode(m, s, wetL, wetR);

    _limiter.processStereo(wetL, wetR);
    _telemetryLimiterGain = _limiter.getGain();

    _softSat.processStereo(wetL, wetR);

    wetL *= outTrimLin;
    wetR *= outTrimLin;

    outL[i] = clampf(wetL, -kOutputClamp, kOutputClamp);
    outR[i] = clampf(wetR, -kOutputClamp, kOutputClamp);
  }

  _limiterTelemetry = _limiter.getTelemetry();
}

float HostHorizonProcessor::getBlockWidth() const {
  return _telemetryWidth;
}

float HostHorizonProcessor::getBlockTransient() const {
  return _telemetryTransient;
}

float HostHorizonProcessor::getLimiterGain() const {
  return _telemetryLimiterGain;
}

float HostHorizonProcessor::getLimiterGRdB() const {
  return _limiter.getGRdB();
}

LimiterLookahead::Telemetry HostHorizonProcessor::getLimiterTelemetry() const {
  return _limiterTelemetry;
}

bool HostHorizonProcessor::getLimiterClipFlagAndClear() {
  return _limiter.getClipFlagAndClear();
}
