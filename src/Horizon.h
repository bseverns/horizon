#pragma once
#include "Arduino.h"
#include <Audio.h>
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
  void setMix(float m);

  // Telemetry for scope/debug builds
  float getBlockWidth() const;
  float getBlockTransient() const;
  float getLimiterGain() const;

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
  float _mixTarget;

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
  ParamSmoother _mixSm;

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
};
