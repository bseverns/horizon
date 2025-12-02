#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "LimiterLookahead.h"

struct HorizonParams {
  float width = 0.6f;
  float dynWidth = 0.35f;
  float transientSens = 0.5f;
  float midTiltDbPerOct = 0.0f;
  float sideAirFreqHz = 10000.0f;
  float sideAirGainDb = 2.0f;
  float lowAnchorHz = 100.0f;
  float dirtAmount = 0.1f;
  float ceilingDb = -1.0f;
  float limiterReleaseMs = 80.0f;
  float limiterLookaheadMs = 5.8f;
  float limiterTiltDbPerOct = 0.0f;
  LimiterLookahead::LinkMode limiterLink = LimiterLookahead::LinkMode::Linked;
  float limiterMix = 0.7f;
  bool limiterBypass = false;
  float outputTrimDb = 0.0f;
};

struct StereoBuffer {
  int sampleRate = 44100;
  std::vector<float> left;
  std::vector<float> right;
};

struct WavRender {
  std::string flavor;
  HorizonParams params;
  StereoBuffer audio;
};

class HostProcessor {
public:
  HostProcessor();

  void reset();
  void setParameters(const HorizonParams &params);
  StereoBuffer process(const StereoBuffer &input);

private:
  HorizonParams _params;
};

StereoBuffer loadStereoWav(const std::filesystem::path &path);
void writeStereoWav(const std::filesystem::path &path, const StereoBuffer &buffer);

std::vector<WavRender> buildDemoRenders(const StereoBuffer &input);

