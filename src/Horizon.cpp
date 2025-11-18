#include "Horizon.h"

void AudioHorizon::setWidth(float w) {(void)w;}
void AudioHorizon::setDynWidth(float a) {(void)a;}
void AudioHorizon::setTransientSens(float s) {(void)s;}
void AudioHorizon::setMidTilt(float dBPerOct) {(void)dBPerOct;}
void AudioHorizon::setSideAir(float freq, float gain) {(void)freq; (void)gain;}
void AudioHorizon::setLowAnchor(float hz) {(void)hz;}
void AudioHorizon::setDirt(float amt) {(void)amt;}
void AudioHorizon::setCeiling(float dB) {(void)dB;}
void AudioHorizon::setMix(float m) {(void)m;}

void AudioHorizon::update() {
  audio_block_t* inL = receiveReadOnly(0);
  audio_block_t* inR = receiveReadOnly(1);
  if (!inL || !inR) { if(inL) release(inL); if(inR) release(inR); return; }
  audio_block_t* outL = allocate();
  audio_block_t* outR = allocate();
  if (!outL || !outR) { if(outL) release(outL); if(outR) release(outR); release(inL); release(inR); return; }

  // TODO: M/S encode → EQs → transient detect → dyn width → softsat → limiter → width → mix
  // For now, pass input to output.
  memcpy(outL->data, inL->data, sizeof(outL->data));
  memcpy(outR->data, inR->data, sizeof(outR->data));

  transmit(outL, 0); transmit(outR, 1);
  release(inL); release(inR); release(outL); release(outR);
}