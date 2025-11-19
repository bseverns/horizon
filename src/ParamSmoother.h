#pragma once

class ParamSmoother {
public:
  explicit ParamSmoother(float alpha = 0.2f)
    : _alpha(alpha), _value(0.0f), _initialized(false) {}

  void setSmoothing(float alpha);

  void reset(float v);

  float process(float target);

private:
  float _alpha;
  float _value;
  bool _initialized;
};
