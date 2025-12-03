// HORIZON transient clamp + tail bloom lab — listen for the stage breathing
// This sketch focuses on how width responds to hits vs. decays. It cycles
// through scenes that pinch the stereo field on transients, then let the
// reverb tails stretch back out. Timing is borrowed from preset_morph_walk:
// hang on a scene, glide to the next, repeat.
//
// Wiring: Teensy 4.x + SGTL5000 audio shield, I2S in/out. Plug in stereo
// material with a bit of ambience and watch Serial for the scene captions.

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
    "Clamp & Breathe",
    "Clamp hits, relax tails — bus_glue bones with hotter transient grab.",
    {
      0.70f,   // width: lean narrower than Bus Glue
      0.78f,   // dyn width: strong transient swing
      0.70f,   // transient sensitivity: clamp quickly
      0.30f,   // mid tilt dB/oct: nudge presence
      9000.0f, // side air freq
      1.2f,    // side air gain dB
      160.0f,  // low anchor Hz: keep kicks glued to center
      0.12f,   // dirt
      -1.2f,   // limiter ceiling dB
      0.9f     // mix
    },
    7000,
    4000
  },
  {
    "Bloom Afterglow",
    "Tails widen and shimmer once the smack fades.",
    {
      1.20f,   // width: stretch
      0.65f,   // dyn width: motion tied to envelope
      0.55f,   // transient sensitivity: still listening, but looser
      11500.0f,// side air freq
      3.5f,    // side air gain dB
      95.0f,   // low anchor Hz: loosen the center a hair
      0.18f,   // dirt
      -1.5f,   // limiter ceiling dB
      0.9f     // mix
    },
    7000,
    4000
  },
  {
    "Anchor Smasher",
    "Limiter leans in; anchor stays glued while sides bloom post-hit.",
    {
      1.35f,   // width: aggressively wide
      0.88f,   // dyn width: heavy transient swing
      0.75f,   // transient sensitivity: pump the detector
      -0.10f,  // mid tilt dB/oct: back off the mids a bit
      10500.0f,// side air freq
      2.0f,    // side air gain dB
      185.0f,  // low anchor Hz: bold center hold
      0.20f,   // dirt
      -2.5f,   // limiter ceiling dB: audible clamp
      0.85f    // mix
    },
    7000,
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

  sStartPreset  = scene.preset;
  sTargetPreset = next.preset;
  sSceneClock   = 0;

  pushPresetToHorizon(scene.preset);

  Serial.println();
  Serial.print("Scene: ");
  Serial.println(scene.name);
  Serial.print("Note: ");
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
    // Stay parked; smoothing keeps the tweaks musical.
    return;
  }

  if (elapsed < scene.holdMs + scene.morphMs) {
    float t = (static_cast<float>(elapsed - scene.holdMs) /
               static_cast<float>(scene.morphMs));

    HorizonPreset morph;
    morph.width           = lerp(sStartPreset.width, sTargetPreset.width, t);
    morph.dynWidth        = lerp(sStartPreset.dynWidth, sTargetPreset.dynWidth, t);
    morph.transientSens   = lerp(sStartPreset.transientSens, sTargetPreset.transientSens, t);
    morph.midTiltDbPerOct = lerp(sStartPreset.midTiltDbPerOct, sTargetPreset.midTiltDbPerOct, t);
    morph.airFreqHz       = lerp(sStartPreset.airFreqHz, sTargetPreset.airFreqHz, t);
    morph.airGainDb       = lerp(sStartPreset.airGainDb, sTargetPreset.airGainDb, t);
    morph.lowAnchorHz     = lerp(sStartPreset.lowAnchorHz, sTargetPreset.lowAnchorHz, t);
    morph.dirt            = lerp(sStartPreset.dirt, sTargetPreset.dirt, t);
    morph.ceilingDb       = lerp(sStartPreset.ceilingDb, sTargetPreset.ceilingDb, t);
    morph.mix             = lerp(sStartPreset.mix, sTargetPreset.mix, t);

    pushPresetToHorizon(morph);
    return;
  }

  selectScene(sSceneIndex + 1);
}
