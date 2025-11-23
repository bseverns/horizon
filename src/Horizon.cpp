#include "Horizon.h"
#include <math.h>

static inline float clampf_hz(float x, float lo, float hi) {
  return (x < lo) ? lo : (x > hi ? hi : x);
}

static inline audio_block_t** clear_input_queue(audio_block_t* (&queue)[2]) {
  queue[0] = nullptr;
  queue[1] = nullptr;
  return queue;
}

static constexpr float kInv32768 = 1.0f / 32768.0f;
static constexpr float kLimiterCeilingMinDb = -12.0f;
static constexpr float kLimiterCeilingMaxDb = -0.1f;

AudioHorizon::AudioHorizon()
  : AudioStream(2, clear_input_queue(inputQueueArray)),
    _widthTarget(0.6f),
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

void AudioHorizon::setWidth(float w) {
  _widthTarget = clampf_hz(w, 0.0f, 1.0f);
}

void AudioHorizon::setDynWidth(float a) {
  _dynWidthTarget = clampf_hz(a, 0.0f, 1.0f);
}

void AudioHorizon::setTransientSens(float s) {
  _transientSensTarget = clampf_hz(s, 0.0f, 1.0f);
}

void AudioHorizon::setMidTilt(float dBPerOct) {
  _midTiltTarget = clampf_hz(dBPerOct, -6.0f, 6.0f);
}

void AudioHorizon::setSideAir(float freqHz, float gainDb) {
  _sideAirFreqTarget = clampf_hz(freqHz, 4000.0f, 16000.0f);
  _sideAirGainTarget = clampf_hz(gainDb, -6.0f, 6.0f);
}

void AudioHorizon::setLowAnchor(float hz) {
  _lowAnchorHzTarget = clampf_hz(hz, 40.0f, 250.0f);
}

void AudioHorizon::setDirt(float amt) {
  _dirtTarget = clampf_hz(amt, 0.0f, 1.0f);
}

void AudioHorizon::setCeiling(float dB) {
  _ceilingDbTarget = clampf_hz(dB, kLimiterCeilingMinDb, kLimiterCeilingMaxDb);
}

void AudioHorizon::setLimiterReleaseMs(float ms) {
  _limiterReleaseTargetMs = clampf_hz(ms, 20.0f, 200.0f);
}

void AudioHorizon::setLimiterLookaheadMs(float ms) {
  _limiterLookaheadTargetMs = clampf_hz(ms, 1.0f, 8.0f);
}

void AudioHorizon::setLimiterDetectorTilt(float dBPerOct) {
  _limiterTiltTarget = clampf_hz(dBPerOct, -3.0f, 3.0f);
}

void AudioHorizon::setLimiterLinkMode(LimiterLookahead::LinkMode mode) {
  _limiterLinkModeTarget = mode;
}

void AudioHorizon::setLimiterMix(float m) {
  _limiterMixTarget = clampf_hz(m, 0.0f, 1.0f);
}

void AudioHorizon::setLimiterBypass(bool on) {
  _limiterBypassTarget = on;
}

void AudioHorizon::setMix(float m) {
  _mixTarget = clampf_hz(m, 0.0f, 1.0f);
  _limiterMixTarget = _mixTarget;
}

void AudioHorizon::setOutputTrim(float dB) {
  _outTrimDbTarget = clampf_hz(dB, -12.0f, 6.0f);
}

void AudioHorizon::update() {
  audio_block_t* inL = receiveReadOnly(0);
  audio_block_t* inR = receiveReadOnly(1);
  if (!inL || !inR) {
    if (inL) release(inL);
    if (inR) release(inR);
    return;
  }

  audio_block_t* outL = allocate();
  audio_block_t* outR = allocate();
  if (!outL || !outR) {
    if (outL) release(outL);
    if (outR) release(outR);
    release(inL);
    release(inR);
    return;
  }

  // Smooth parameters for this block.
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
  float outTrimLin = powf(10.0f, 0.05f * outTrimDb);

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

  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) {
    float l = inL->data[i] * kInv32768;
    float r = inR->data[i] * kInv32768;

    float m, s;
    _ms.encode(l, r, m, s);

    m = _midTilt.processSample(m);
    s = _sideAir.processSample(s);

    float detectorIn = 0.5f * (fabsf(m) + fabsf(s));
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

    float outLf = wetL;
    float outRf = wetR;

    float scaledL = outLf * 32767.0f;
    float scaledR = outRf * 32767.0f;

    if (scaledL > 32767.0f) scaledL = 32767.0f;
    if (scaledL < -32767.0f) scaledL = -32767.0f;
    if (scaledR > 32767.0f) scaledR = 32767.0f;
    if (scaledR < -32767.0f) scaledR = -32767.0f;

    outL->data[i] = static_cast<int16_t>(scaledL);
    outR->data[i] = static_cast<int16_t>(scaledR);
  }

  _limiterTelemetry = _limiter.getTelemetry();

  transmit(outL, 0);
  transmit(outR, 1);
  release(inL);
  release(inR);
  release(outL);
  release(outR);
}

float AudioHorizon::getBlockWidth() const {
  return _telemetryWidth;
}

float AudioHorizon::getBlockTransient() const {
  return _telemetryTransient;
}

float AudioHorizon::getLimiterGain() const {
  return _telemetryLimiterGain;
}

float AudioHorizon::getLimiterGRdB() const {
  return _limiter.getGRdB();
}

LimiterLookahead::Telemetry AudioHorizon::getLimiterTelemetry() const {
  return _limiterTelemetry;
}

bool AudioHorizon::getLimiterClipFlagAndClear() {
  return _limiter.getClipFlagAndClear();
}
