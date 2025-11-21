#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include "Horizon.h"

// Audio objects
AudioInputI2S            audioInput;   // from SGTL5000 or line-in
AudioHorizon             horizon;
AudioOutputI2S           audioOutput;  // to SGTL5000 headphones/line-out

AudioConnection          patchCord1(audioInput, 0, horizon, 0);
AudioConnection          patchCord2(audioInput, 1, horizon, 1);
AudioConnection          patchCord3(horizon, 0, audioOutput, 0);
AudioConnection          patchCord4(horizon, 1, audioOutput, 1);

AudioControlSGTL5000     sgtl5000_1;

elapsedMillis            printTimer;

#ifdef HORIZON_BUILD_SCOPE
static void printBar(const char* label, float value) {
  if (value < 0.0f) value = 0.0f;
  if (value > 1.0f) value = 1.0f;

  const int width = 32;
  int filled = (int)(value * width + 0.5f);
  if (filled < 0) filled = 0;
  if (filled > width) filled = width;

  Serial.print(label);
  Serial.print(" |");
  for (int i = 0; i < width; ++i) {
    Serial.print(i < filled ? '#' : ' ');
  }
  Serial.print("| ");
  Serial.println(value, 3);
}
#endif

void setup() {
  delay(1000);              // let USB enumerate
  Serial.begin(115200);
  AudioMemory(40);

  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5f);

  // A starting point preset: moderate width, some dynamics, gentle dirt.
  horizon.setWidth(0.6f);
  horizon.setDynWidth(0.4f);
  horizon.setTransientSens(0.5f);
  horizon.setMidTilt(0.0f);
  horizon.setSideAir(10000.0f, 2.0f);
  horizon.setLowAnchor(100.0f);
  horizon.setDirt(0.15f);
  horizon.setCeiling(-1.0f);
  horizon.setMix(0.7f);
}

void loop()  {
#ifdef HORIZON_BUILD_SCOPE
  // --- SCOPE VERSION ---
  // The ASCII bar-graph telemetry from the scope sketch:
  // Print telemetry ~10 times a second.
  if (printTimer > 100) {
    float width  = horizon.getBlockWidth();      // 0..1 approx
    float trans  = horizon.getBlockTransient();  // 0..1
    float gain   = horizon.getLimiterGain();     // 0..1 linear

    Serial.println();
    printBar("Width    ", width);
    printBar("Transient", trans);

    // Limiter gain is linear; map it into a 0..1-ish bar:
    // 1.0  -> no limiting
    // 0.25 -> heavy limiting
    float limBar = gain;
    if (limBar < 0.0f) limBar = 0.0f;
    if (limBar > 1.0f) limBar = 1.0f;
    printBar("Limiter  ", limBar);

    printTimer = 0;
  }
#else
  // --- MAIN VERSION ---
  // No Serial printing, just whatever control / UI you want:
  // - read knobs, buttons, encoders
  // - map to horizon.setWidth(), setDynWidth(), etc.
  // - maybe occasional debug prints, but not the full scope.

#endif
}