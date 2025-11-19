#pragma once

class SoftSaturation {
public:
  SoftSaturation();

  // amt 0..1; 0 ~= bypass, 1 = strong.
  void setAmount(float amt);

  float processSample(float x) const;

  void processStereo(float& l, float& r) const;

private:
  float _amount;
  float _drive;
  float _invTanhDrive;
};
