#pragma once

class ParamSmoother {
public:
  explicit ParamSmoother(float alpha = 0.2f)
    : _alpha(alpha), _value(0.0f), _initialized(false) {}

  void setSmoothing(float alpha);

  // Set a time constant in milliseconds, recomputing alpha for a given
  // sample/update interval. `samplesPerUpdate` is how often `process` will be
  // called (e.g. host block size if smoothing per-block).
  void setTimeConstantMs(float ms, double sampleRate, int samplesPerUpdate);

  void reset(float v);

  float process(float target);

private:
  float _alpha;
  float _value;
  bool _initialized;
};
