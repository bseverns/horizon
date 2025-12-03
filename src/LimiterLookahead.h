#pragma once
#include <Arduino.h>
#include "TiltEQ.h"
#include "TransientDetector.h"
#include "SoftSaturation.h"

// See docs/block_notes.md for musician-facing notes on this block.

// Lookahead limiter with detector tilt, adaptive release, and click-free bypass.
// The limiter owns its own delay line so dry/wet mixes stay time-aligned.
class LimiterLookahead {
public:
  enum class LinkMode { Linked, MidSide };

  struct Telemetry {
    float env_db;
    float peak_in;
    float peak_out;
  };

  LimiterLookahead();

  void setup();

  // Configuration (all clamped to safe musical ranges)
  void setCeilingDb(float db);             // -12 .. -0.1 dBFS
  void setReleaseMs(float ms);             // 20 .. 200 ms
  void setLookaheadMs(float ms);           // 1 .. 8 ms
  void setDetectorTiltDbPerOct(float db);  // -3 .. +3 dB/oct
  void setLinkMode(LinkMode mode);
  void setMix(float mix01);                // 0..1, 0=dry, 1=wet
  void setBypass(bool on);                 // smooth 5 ms crossfade
  void setSampleRate(float sampleRate);

  void reset();

  // In-place stereo sample processing, -1..1 domain.
  void processStereo(float& l, float& r);

  float getGain() const { return _env; }
  float getGRdB() const;
  bool getClipFlagAndClear();
  Telemetry getTelemetry() const { return _lastTelemetry; }

private:
  static constexpr int kBufferSize = 4096;
  static constexpr float kEps = 1e-12f;

  static float dbToLin(float dB);
  void updateReleaseCoeff();
  void updateLookaheadSamples();
  void updateXfade();
  void finalizeBlock();
  void primeDelayLine();

  float _bufferL[kBufferSize];
  float _bufferR[kBufferSize];
  int _writeIdx;
  int _lookaheadSamples;
  float _lookaheadMs;
  float _mix;

  float _ceilingDb;
  float _ceilingLin;
  float _releaseMs;
  float _releaseCoeff;
  float _env;

  float _sampleRate;

  TiltEQ _detectorTiltL;
  TiltEQ _detectorTiltR;
  TransientDetector _transientDetector;
  float _lastTransientAvg;

  LinkMode _linkMode;

  SoftSaturation _safetyClip;
  bool _clipFlag;

  // Bypass crossfade
  bool _bypassTarget;
  float _bypassMix;
  int _xfadeSamples;
  int _xfadeCountdown;

  // Block telemetry helpers
  int _blockPos;
  float _blockPeakIn;
  float _blockPeakOut;
  float _blockTransientSum;
  Telemetry _lastTelemetry;
};

// Map gain reduction dB to an 8-step LED ladder (higher index = more GR)
uint8_t mapGRtoLeds(float gr_db);
