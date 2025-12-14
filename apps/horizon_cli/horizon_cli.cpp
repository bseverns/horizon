#include <sndfile.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "host/HostHorizonProcessor.h"

namespace {

struct AudioFile {
  std::vector<float> samples;
  int sampleRate = 44100;
  int channels   = 2;
};

// Simple preset map that mirrors the JSON files under presets/.
struct Preset {
  const char* name;
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

const std::map<std::string, Preset> kPresets = {
    {"bus_glue",
     {"Bus Glue", 0.72f, 0.32f, 0.25f, 0.35f, 9000.0f, 1.0f, 140.0f, 0.10f, -1.2f, 0.85f}},
    {"crunch_room",
     {"Crunch Room", 0.55f, 0.42f, 0.40f, 0.0f, 10500.0f, 2.5f, 120.0f, 0.22f, -2.5f, 0.78f}},
    {"width_extremes",
     {"Width Extremes", 0.92f, 0.65f, 0.30f, 0.25f, 8000.0f, 1.8f, 120.0f, 0.12f, -1.8f, 0.90f}},
};

std::string to_lower(std::string s) {
  for (char& c : s) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return s;
}

bool load_audio(const std::string& path, AudioFile& out) {
  SF_INFO info{};
  SNDFILE* file = sf_open(path.c_str(), SFM_READ, &info);
  if (!file) {
    std::cerr << "[horizon-cli] Failed to open input: " << path << "\n";
    return false;
  }

  if (info.channels < 1 || info.channels > 2) {
    std::cerr << "[horizon-cli] Only mono/stereo files supported. Got " << info.channels << " channels.\n";
    sf_close(file);
    return false;
  }

  out.channels   = (info.channels == 1) ? 2 : info.channels;
  out.sampleRate = info.samplerate;
  out.samples.resize(static_cast<size_t>(info.frames) * static_cast<size_t>(out.channels));

  std::vector<float> temp(static_cast<size_t>(info.frames) * static_cast<size_t>(info.channels));
  sf_count_t read = sf_readf_float(file, temp.data(), info.frames);
  sf_close(file);
  if (read != info.frames) {
    std::cerr << "[horizon-cli] Short read from file: " << path << "\n";
    return false;
  }

  // De-interleave mono to stereo if needed, otherwise copy straight through.
  for (sf_count_t frame = 0; frame < info.frames; ++frame) {
    float l = temp[frame * info.channels];
    float r = (info.channels == 1) ? l : temp[frame * info.channels + 1];
    out.samples[2 * static_cast<size_t>(frame) + 0] = l;
    out.samples[2 * static_cast<size_t>(frame) + 1] = r;
  }

  return true;
}

bool write_audio(const std::string& path, const AudioFile& data) {
  SF_INFO info{};
  info.channels   = 2;
  info.samplerate = data.sampleRate;
  info.format     = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

  // Pick a container based on the extension; default WAV keeps the build simple.
  std::string lower = to_lower(path);
  if (lower.rfind(".flac") != std::string::npos) {
    info.format = SF_FORMAT_FLAC | SF_FORMAT_FLOAT;
  } else if (lower.rfind(".aiff") != std::string::npos || lower.rfind(".aif") != std::string::npos) {
    info.format = SF_FORMAT_AIFF | SF_FORMAT_FLOAT;
  }

  SNDFILE* file = sf_open(path.c_str(), SFM_WRITE, &info);
  if (!file) {
    std::cerr << "[horizon-cli] Failed to open output for write: " << path << "\n";
    return false;
  }

  const sf_count_t frames = static_cast<sf_count_t>(data.samples.size() / 2);
  sf_count_t written      = sf_writef_float(file, data.samples.data(), frames);
  sf_write_sync(file);
  sf_close(file);

  if (written != frames) {
    std::cerr << "[horizon-cli] Short write on output: " << path << "\n";
    return false;
  }
  return true;
}

Preset pick_preset(const std::string& name) {
  std::string key = to_lower(name);
  auto it         = kPresets.find(key);
  if (it != kPresets.end()) {
    return it->second;
  }
  throw std::runtime_error("Unknown preset: " + name);
}

void print_presets() {
  std::cout << "Available presets:\n";
  for (const auto& entry : kPresets) {
    std::cout << "  - " << entry.first << " (" << entry.second.name << ")\n";
  }
}

void apply_preset(HostHorizonProcessor& proc, const Preset& p) {
  proc.setWidth(p.width);
  proc.setDynWidth(p.dynWidth);
  proc.setTransientSens(p.transientSens);
  proc.setMidTilt(p.midTiltDbPerOct);
  proc.setSideAir(p.airFreqHz, p.airGainDb);
  proc.setLowAnchor(p.lowAnchorHz);
  proc.setDirt(p.dirt);
  proc.setCeiling(p.ceilingDb);
  proc.setMix(p.mix);
}

AudioFile render_with_horizon(const AudioFile& input, const Preset& preset, int blockSize) {
  AudioFile out;
  out.channels   = 2;
  out.sampleRate = input.sampleRate;
  out.samples.resize(input.samples.size());

  HostHorizonProcessor horizon(static_cast<double>(input.sampleRate), blockSize);
  apply_preset(horizon, preset);

  std::vector<float> inL(blockSize, 0.0f);
  std::vector<float> inR(blockSize, 0.0f);
  std::vector<float> outL(blockSize, 0.0f);
  std::vector<float> outR(blockSize, 0.0f);

  const size_t totalFrames = input.samples.size() / 2;
  for (size_t frame = 0; frame < totalFrames; frame += static_cast<size_t>(blockSize)) {
    const size_t framesThisBlock = std::min(static_cast<size_t>(blockSize), totalFrames - frame);
    std::fill(inL.begin(), inL.end(), 0.0f);
    std::fill(inR.begin(), inR.end(), 0.0f);

    for (size_t i = 0; i < framesThisBlock; ++i) {
      inL[i] = input.samples[2 * (frame + i) + 0];
      inR[i] = input.samples[2 * (frame + i) + 1];
    }

    horizon.processBlock(inL.data(), inR.data(), outL.data(), outR.data(),
                         static_cast<int>(framesThisBlock), static_cast<double>(input.sampleRate));

    for (size_t i = 0; i < framesThisBlock; ++i) {
      out.samples[2 * (frame + i) + 0] = outL[i];
      out.samples[2 * (frame + i) + 1] = outR[i];
    }
  }

  return out;
}

void print_usage(const char* argv0) {
  std::cout << "horizon-cli <input.(wav|aiff|flac)> <output.(wav|aiff|flac)> [--preset name] [--block N]\n";
  std::cout << "  --preset name   One of: bus_glue, crunch_room, width_extremes (defaults to bus_glue)\n";
  std::cout << "  --block N       Override block size (default 512).\n";
  std::cout << "  --list-presets  Print the preset menu and exit.\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  std::string inputPath;
  std::string outputPath = "horizon_out.wav";
  std::string presetName = "bus_glue";
  int blockSize          = 512;

  inputPath = argv[1];
  if (argc >= 3) {
    outputPath = argv[2];
  }

  for (int i = 3; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--preset" && i + 1 < argc) {
      presetName = argv[++i];
    } else if (arg == "--block" && i + 1 < argc) {
      blockSize = std::max(16, std::atoi(argv[++i]));
    } else if (arg == "--list-presets") {
      print_presets();
      return 0;
    }
  }

  AudioFile input;
  if (!load_audio(inputPath, input)) {
    return 1;
  }

  Preset preset{};
  try {
    preset = pick_preset(presetName);
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    print_presets();
    return 1;
  }

  AudioFile output = render_with_horizon(input, preset, blockSize);
  if (!write_audio(outputPath, output)) {
    return 1;
  }

  std::cout << "[horizon-cli] Rendered " << (output.samples.size() / 2) << " frames using preset '"
            << presetName << "' → " << outputPath << "\n";
  return 0;
}
