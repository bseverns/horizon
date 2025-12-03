#pragma once
#include "Arduino.h"
#include <Audio.h>
#include <AudioStream.h>
#include "MSMatrix.h"
#include "TiltEQ.h"
#include "AirEQ.h"
#include "DynWidth.h"
#include "TransientDetector.h"
#include "SoftSaturation.h"
#include "LimiterLookahead.h"
#include "ParamSmoother.h"

class AudioHorizon : public AudioStream {
public:
  AudioHorizon();

  void setWidth(float w);
  void setDynWidth(float a);
  void setTransientSens(float s);
  void setMidTilt(float dBPerOct);
  void setSideAir(float freqHz, float gainDb);
  void setLowAnchor(float hz);
  void setDirt(float amt);
  void setCeiling(float dB);
  void setLimiterReleaseMs(float ms);
  void setLimiterLookaheadMs(float ms);
  void setLimiterDetectorTilt(float dBPerOct);
  void setLimiterLinkMode(LimiterLookahead::LinkMode mode);
  void setLimiterMix(float m);
  void setLimiterBypass(bool on);
  void setMix(float m);
  void setOutputTrim(float dB);

  // Telemetry for scope/debug builds
  float getBlockWidth() const;
  float getBlockTransient() const;
  float getLimiterGain() const;
  float getLimiterGRdB() const;
  LimiterLookahead::Telemetry getLimiterTelemetry() const;
  bool getLimiterClipFlagAndClear();

  virtual void update() override;

private:
  audio_block_t* inputQueueArray[2]{nullptr, nullptr};

  // Parameter targets
  float _widthTarget;
  float _dynWidthTarget;
  float _transientSensTarget;
  float _midTiltTarget;
  float _sideAirFreqTarget;
  float _sideAirGainTarget;
  float _lowAnchorHzTarget;
  float _dirtTarget;
  float _ceilingDbTarget;
  float _limiterReleaseTargetMs;
  float _limiterLookaheadTargetMs;
  float _limiterTiltTarget;
  float _limiterMixTarget;
  LimiterLookahead::LinkMode _limiterLinkModeTarget;
  bool _limiterBypassTarget;
  float _mixTarget;
  float _outTrimDbTarget;

  // Smoothers
  ParamSmoother _widthSm;
  ParamSmoother _dynWidthSm;
  ParamSmoother _transientSm;
  ParamSmoother _midTiltSm;
  ParamSmoother _sideAirFreqSm;
  ParamSmoother _sideAirGainSm;
  ParamSmoother _lowAnchorSm;
  ParamSmoother _dirtSm;
  ParamSmoother _ceilingSm;
  ParamSmoother _limiterReleaseSm;
  ParamSmoother _limiterLookaheadSm;
  ParamSmoother _limiterTiltSm;
  ParamSmoother _limiterMixSm;
  ParamSmoother _mixSm;
  ParamSmoother _outTrimSm;

  // DSP blocks
  MSMatrix _ms;
  TiltEQ _midTilt;
  AirEQ _sideAir;
  DynWidth _dynWidth;
  TransientDetector _detector;
  SoftSaturation _softSat;
  LimiterLookahead _limiter;

  // Cached telemetry
  float _telemetryWidth;
  float _telemetryTransient;
  float _telemetryLimiterGain;
  LimiterLookahead::Telemetry _limiterTelemetry;
};
