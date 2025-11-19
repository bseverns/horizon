#pragma once

class TransientDetector {
public:
  TransientDetector();

  void reset();

  // sensitivity 0..1 (higher = more transient-sensitive, lower threshold)
  void setSensitivity(float s);

  // x in -1..1; returns transient activity 0..1
  float processSample(float x);

private:
  float _env;
  float _sensitivity;
  float _attackCoeff;
  float _releaseCoeff;
};
