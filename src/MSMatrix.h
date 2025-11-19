#pragma once

class MSMatrix {
public:
  inline void encode(float l, float r, float& m, float& s) const {
    m = 0.5f * (l + r);
    s = 0.5f * (l - r);
  }
  inline void decode(float m, float s, float& l, float& r) const {
    l = m + s;
    r = m - s;
  }
};
