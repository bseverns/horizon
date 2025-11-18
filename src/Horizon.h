#pragma once
#include <Audio.h>

class AudioHorizon : public AudioStream {
public:
  AudioHorizon() : AudioStream(2, inputQueueArray) {}
  void setWidth(float w);
  void setDynWidth(float a);
  void setTransientSens(float s);
  void setMidTilt(float dBPerOct);
  void setSideAir(float freq, float gain);
  void setLowAnchor(float hz);
  void setDirt(float amt);
  void setCeiling(float dB);
  void setMix(float m);
  virtual void update();
private:
  audio_block_t *inputQueueArray[2];
  // TODO: state: ms matrix, filters, detector, limiter, smoothers
};