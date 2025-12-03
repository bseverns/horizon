// HORIZON preset walkabout — timed morphs through bus glue + crunchy room tones
// This sketch is a slow, serial-guided tour through a couple of scene presets.
// Each stop hangs out long enough for your ears to acclimate, then glides to
// the next vibe without a click. Think of it as a lazy DJ crossfader between
// “safe glue” and “clubby grit”, with Serial narrating the mood so students can
// map what they hear to the parameter shifts.
//
// Wiring matches the other Horizon examples: Teensy 4.x + SGTL5000 audio shield
// with I2S in/out. No knobs needed—just listen and watch Serial.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "Horizon.h"

AudioInputI2S        i2sIn;
AudioHorizon         horizon;
AudioOutputI2S       i2sOut;
AudioConnection      patchCord1(i2sIn, 0, horizon, 0);
AudioConnection      patchCord2(i2sIn, 1, horizon, 1);
AudioConnection      patchCord3(horizon, 0, i2sOut, 0);
AudioConnection      patchCord4(horizon, 1, i2sOut, 1);
AudioControlSGTL5000 codec;

struct HorizonPreset {
  float width;
  float dynWidth;
  float transientSens;
  float midTiltDbPerOct;
  float airFreqHz;
  float airGainDb;
  float lowAnchorHz;
  float dirt;
  float ceilingDb;
  float mix;
};

struct PresetScene {
  const char* name;
  const char* descriptor;
  HorizonPreset preset;
  uint32_t holdMs;
  uint32_t morphMs;
};

static const PresetScene kScenes[] = {
  {
    "Bus Glue",
    "Bus Glue — hug the center, sprinkle air, keep the ceiling honest.",
    {
      0.72f,   // width
      0.32f,   // dyn width
      0.40f,   // transient sensitivity
      0.35f,   // mid tilt dB/oct
      9000.0f, // side air freq
      1.0f,    // side air gain dB
      140.0f,  // low anchor Hz
      0.10f,   // dirt
      -1.2f,   // limiter ceiling dB
      0.85f    // mix
    },
    6500,
    4000
  },
  {
    "Crunch Room",
    "Crunch Room — tall, slightly grimy room tone with a snarl up top.",
    {
      0.95f,
      0.55f,
      0.55f,
      -0.15f,
      11000.0f,
      2.5f,
      110.0f,
      0.28f,
      -1.8f,
      0.90f
    },
    6500,
    4000
  },
  {
    "Ambient Glass",
    "Ambient Glass — long hall sparkle, let the sides bloom then chill.",
    {
      1.00f,
      0.70f,
      0.48f,
      0.10f,
      12000.0f,
      6.0f,
      90.0f,
      0.16f,
      -1.5f,
      0.90f
    },
    6500,
    4000
  }
};

static constexpr int kNumScenes = sizeof(kScenes) / sizeof(kScenes[0]);
static int           sSceneIndex = 0;
static HorizonPreset sStartPreset;
static HorizonPreset sTargetPreset;
static elapsedMillis sSceneClock;

static float lerp(float a, float b, float t) {
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  return a + (b - a) * t;
}

static void pushPresetToHorizon(const HorizonPreset& preset) {
  horizon.setWidth(preset.width);
  horizon.setDynWidth(preset.dynWidth);
  horizon.setTransientSens(preset.transientSens);
  horizon.setMidTilt(preset.midTiltDbPerOct);
  horizon.setSideAir(preset.airFreqHz, preset.airGainDb);
  horizon.setLowAnchor(preset.lowAnchorHz);
  horizon.setDirt(preset.dirt);
  horizon.setCeiling(preset.ceilingDb);
  horizon.setMix(preset.mix);
}

static void selectScene(int index) {
  sSceneIndex = index % kNumScenes;
  const PresetScene& scene = kScenes[sSceneIndex];
  const PresetScene& next  = kScenes[(sSceneIndex + 1) % kNumScenes];

  sStartPreset = scene.preset;
  sTargetPreset = next.preset;
  sSceneClock = 0;

  pushPresetToHorizon(scene.preset);

  Serial.println();
  Serial.print("Now live: ");
  Serial.println(scene.name);
  Serial.print("Vibe: ");
  Serial.println(scene.descriptor);
  Serial.println("---");
}

void setup() {
  delay(1000);
  Serial.begin(115200);

  AudioMemory(56);

  codec.enable();
  codec.inputSelect(AUDIO_INPUT_LINEIN);
  codec.volume(0.55f);

  selectScene(0);
}

void loop() {
  const PresetScene& scene = kScenes[sSceneIndex];
  const uint32_t elapsed = sSceneClock;

  if (elapsed < scene.holdMs) {
    // Cruise on the current preset; smoothing already keeps the ride soft.
    return;
  }

  if (elapsed < scene.holdMs + scene.morphMs) {
    float t = (static_cast<float>(elapsed - scene.holdMs) /
               static_cast<float>(scene.morphMs));

    HorizonPreset morph;
    morph.width          = lerp(sStartPreset.width, sTargetPreset.width, t);
    morph.dynWidth       = lerp(sStartPreset.dynWidth, sTargetPreset.dynWidth, t);
    morph.transientSens  = lerp(sStartPreset.transientSens, sTargetPreset.transientSens, t);
    morph.midTiltDbPerOct= lerp(sStartPreset.midTiltDbPerOct, sTargetPreset.midTiltDbPerOct, t);
    morph.airFreqHz      = lerp(sStartPreset.airFreqHz, sTargetPreset.airFreqHz, t);
    morph.airGainDb      = lerp(sStartPreset.airGainDb, sTargetPreset.airGainDb, t);
    morph.lowAnchorHz    = lerp(sStartPreset.lowAnchorHz, sTargetPreset.lowAnchorHz, t);
    morph.dirt           = lerp(sStartPreset.dirt, sTargetPreset.dirt, t);
    morph.ceilingDb      = lerp(sStartPreset.ceilingDb, sTargetPreset.ceilingDb, t);
    morph.mix            = lerp(sStartPreset.mix, sTargetPreset.mix, t);

    pushPresetToHorizon(morph);
    return;
  }

  // Morph finished: slide to the next scene and keep the loop rolling.
  selectScene(sSceneIndex + 1);
}
