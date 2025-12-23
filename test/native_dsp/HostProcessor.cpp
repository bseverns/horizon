#include "HostProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#include "Audio.h"
#include "AirEQ.h"
#include "DynWidth.h"
#include "MSMatrix.h"
#include "ParamSmoother.h"
#include "SoftSaturation.h"
#include "TiltEQ.h"
#include "TransientDetector.h"

namespace {

constexpr float kInv32768 = 1.0f / 32768.0f;
constexpr float kInv8388608 = 1.0f / 8388608.0f; // 24-bit signed max

struct WavHeader {
  char riffId[4];
  uint32_t riffSize;
  char waveId[4];
};

struct FmtChunk {
  uint16_t audioFormat = 0;
  uint16_t numChannels = 0;
  uint32_t sampleRate = 0;
  uint32_t byteRate = 0;
  uint16_t blockAlign = 0;
  uint16_t bitsPerSample = 0;
  uint16_t cbSize = 0;
  uint16_t validBitsPerSample = 0;
  uint32_t channelMask = 0;
  uint32_t subFormatData1 = 0; // first 32 bits of GUID; 0x00000001 == PCM
};

int16_t readLE16(std::istream &in) {
  uint8_t bytes[2] = {0};
  in.read(reinterpret_cast<char *>(bytes), 2);
  return static_cast<int16_t>(bytes[0] | (bytes[1] << 8));
}

uint32_t readLE32(std::istream &in) {
  uint8_t bytes[4] = {0};
  in.read(reinterpret_cast<char *>(bytes), 4);
  return static_cast<uint32_t>(bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24));
}

int32_t readLE24(std::istream &in) {
  uint8_t bytes[3] = {0};
  in.read(reinterpret_cast<char *>(bytes), 3);
  int32_t value = static_cast<int32_t>(bytes[0] | (bytes[1] << 8) | (bytes[2] << 16));
  if (value & 0x00800000) {
    value |= 0xFF000000; // sign-extend negative values
  }
  return value;
}

void writeLE16(std::ostream &out, int16_t v) {
  char bytes[2];
  bytes[0] = static_cast<char>(v & 0xFF);
  bytes[1] = static_cast<char>((v >> 8) & 0xFF);
  out.write(bytes, 2);
}

void writeLE32(std::ostream &out, uint32_t v) {
  char bytes[4];
  bytes[0] = static_cast<char>(v & 0xFF);
  bytes[1] = static_cast<char>((v >> 8) & 0xFF);
  bytes[2] = static_cast<char>((v >> 16) & 0xFF);
  bytes[3] = static_cast<char>((v >> 24) & 0xFF);
  out.write(bytes, 4);
}

} // namespace

HostProcessor::HostProcessor() { reset(); }

void HostProcessor::reset() { _params = HorizonParams{}; }

void HostProcessor::setParameters(const HorizonParams &params) { _params = params; }

StereoBuffer HostProcessor::process(const StereoBuffer &input) {
  StereoBuffer out;
  out.sampleRate = input.sampleRate;
  out.left.resize(input.left.size());
  out.right.resize(input.right.size());

  MSMatrix ms;
  TiltEQ midTilt;
  AirEQ sideAir;
  DynWidth dynWidth;
  TransientDetector detector;
  SoftSaturation softSat;
  LimiterLookahead limiter;

  ParamSmoother widthSm(0.08f);
  ParamSmoother dynWidthSm(0.08f);
  ParamSmoother transientSm(0.08f);
  ParamSmoother midTiltSm(0.08f);
  ParamSmoother sideAirFreqSm(0.08f);
  ParamSmoother sideAirGainSm(0.08f);
  ParamSmoother lowAnchorSm(0.08f);
  ParamSmoother dirtSm(0.08f);
  ParamSmoother ceilingSm(0.08f);
  ParamSmoother limiterReleaseSm(0.08f);
  ParamSmoother limiterLookaheadSm(0.08f);
  ParamSmoother limiterTiltSm(0.08f);
  ParamSmoother limiterMixSm(0.08f);
  ParamSmoother outTrimSm(0.1f);

  dynWidth.setBaseWidth(_params.width);
  dynWidth.setDynAmount(_params.dynWidth);
  dynWidth.setLowAnchorHz(_params.lowAnchorHz);
  detector.setSensitivity(_params.transientSens);
  midTilt.setTiltDbPerOct(_params.midTiltDbPerOct);
  sideAir.setFreqAndGain(_params.sideAirFreqHz, _params.sideAirGainDb);
  softSat.setAmount(_params.dirtAmount);
  limiter.setup();
  limiter.setCeilingDb(_params.ceilingDb);
  limiter.setReleaseMs(_params.limiterReleaseMs);
  limiter.setLookaheadMs(_params.limiterLookaheadMs);
  limiter.setDetectorTiltDbPerOct(_params.limiterTiltDbPerOct);
  limiter.setLinkMode(_params.limiterLink);
  limiter.setMix(_params.limiterMix);
  limiter.setBypass(_params.limiterBypass);

  const size_t total = std::min(input.left.size(), input.right.size());
  size_t idx = 0;
  while (idx < total) {
    const size_t block = std::min<size_t>(AUDIO_BLOCK_SAMPLES, total - idx);

    float width = widthSm.process(_params.width);
    float dynAmt = dynWidthSm.process(_params.dynWidth);
    float sens = transientSm.process(_params.transientSens);
    float midTiltDb = midTiltSm.process(_params.midTiltDbPerOct);
    float airFreq = sideAirFreqSm.process(_params.sideAirFreqHz);
    float airGainDb = sideAirGainSm.process(_params.sideAirGainDb);
    float lowAnchorHz = lowAnchorSm.process(_params.lowAnchorHz);
    float dirtAmt = dirtSm.process(_params.dirtAmount);
    float ceilingDb = ceilingSm.process(_params.ceilingDb);
    float limRelease = limiterReleaseSm.process(_params.limiterReleaseMs);
    float limLook = limiterLookaheadSm.process(_params.limiterLookaheadMs);
    float limTilt = limiterTiltSm.process(_params.limiterTiltDbPerOct);
    float limMix = limiterMixSm.process(_params.limiterMix);
    float outTrimDb = outTrimSm.process(_params.outputTrimDb);
    float outTrimLin = powf(10.0f, 0.05f * outTrimDb);

    dynWidth.setBaseWidth(width);
    dynWidth.setDynAmount(dynAmt);
    dynWidth.setLowAnchorHz(lowAnchorHz);
    detector.setSensitivity(sens);
    midTilt.setTiltDbPerOct(midTiltDb);
    sideAir.setFreqAndGain(airFreq, airGainDb);
    softSat.setAmount(dirtAmt);
    limiter.setCeilingDb(ceilingDb);
    limiter.setReleaseMs(limRelease);
    limiter.setLookaheadMs(limLook);
    limiter.setDetectorTiltDbPerOct(limTilt);
    limiter.setLinkMode(_params.limiterLink);
    limiter.setMix(limMix);
    limiter.setBypass(_params.limiterBypass);

    for (size_t i = 0; i < block; ++i) {
      float l = input.left[idx + i];
      float r = input.right[idx + i];

      float m, s;
      ms.encode(l, r, m, s);

      m = midTilt.processSample(m);
      s = sideAir.processSample(s);

      float detectorIn = 0.5f * (fabsf(m) + fabsf(s));
      float activity = detector.processSample(detectorIn);

      dynWidth.processSample(m, s, activity);

      float wetL, wetR;
      ms.decode(m, s, wetL, wetR);

      limiter.processStereo(wetL, wetR);
      softSat.processStereo(wetL, wetR);

      wetL *= outTrimLin;
      wetR *= outTrimLin;

      out.left[idx + i] = wetL;
      out.right[idx + i] = wetR;
    }

    idx += block;
  }

  return out;
}

StereoBuffer loadStereoWav(const std::filesystem::path &path) {
  StereoBuffer buffer;
  std::ifstream in(path, std::ios::binary);
  if (!in.good()) {
    std::cerr << "Could not open WAV: " << path << "\n";
    return buffer;
  }

  WavHeader header{};
  in.read(reinterpret_cast<char *>(&header), sizeof(header));
  if (std::string(header.riffId, 4) != "RIFF" || std::string(header.waveId, 4) != "WAVE") {
    std::cerr << "Invalid WAV header in: " << path << "\n";
    return buffer;
  }

  FmtChunk fmt{};
  bool sawFmt = false;
  uint32_t dataSize = 0;
  std::streampos dataPos = std::streampos(-1);
  while (in.good()) {
    char chunkId[4] = {0};
    in.read(chunkId, 4);
    if (!in.good()) break;
    uint32_t chunkSize = readLE32(in);
    std::string id(chunkId, 4);
    const bool isDataChunk = (id == "data");

    if (id == "fmt ") {
      std::vector<uint8_t> fmtData(chunkSize);
      in.read(reinterpret_cast<char *>(fmtData.data()), chunkSize);

      auto readLE16buf = [&fmtData](size_t offset) {
        return static_cast<uint16_t>(fmtData[offset] | (fmtData[offset + 1] << 8));
      };
      auto readLE32buf = [&fmtData](size_t offset) {
        return static_cast<uint32_t>(fmtData[offset] | (fmtData[offset + 1] << 8) | (fmtData[offset + 2] << 16) |
                                     (fmtData[offset + 3] << 24));
      };

      if (chunkSize >= 16) {
        fmt.audioFormat = readLE16buf(0);
        fmt.numChannels = readLE16buf(2);
        fmt.sampleRate = readLE32buf(4);
        fmt.byteRate = readLE32buf(8);
        fmt.blockAlign = readLE16buf(12);
        fmt.bitsPerSample = readLE16buf(14);
      }

      if (chunkSize >= 18) {
        fmt.cbSize = readLE16buf(16);
      }

      const bool isExtensible = (fmt.audioFormat == 0xFFFE);
      if (isExtensible && chunkSize >= 40) {
        fmt.validBitsPerSample = readLE16buf(18);
        fmt.channelMask = readLE32buf(20);
        fmt.subFormatData1 = readLE32buf(24);
      }

      if (chunkSize % 2 == 1) {
        in.seekg(1, std::ios::cur);
      }

      sawFmt = true;
    } else if (isDataChunk) {
      dataSize = chunkSize;
      dataPos = in.tellg();
      if (sawFmt) {
        break; // We have both fmt + data; go decode samples.
      }

      // Keep scanning for fmt, but hop over the data payload first.
      in.seekg(chunkSize, std::ios::cur);
      if (chunkSize % 2 == 1) {
        in.seekg(1, std::ios::cur);
      }
    } else {
      in.seekg(chunkSize, std::ios::cur);
      if (chunkSize % 2 == 1) {
        in.seekg(1, std::ios::cur);
      }
    }
  }

  const uint16_t bitDepth = fmt.validBitsPerSample ? fmt.validBitsPerSample : fmt.bitsPerSample;
  const bool pcmFormat = (fmt.audioFormat == 1) || (fmt.audioFormat == 0xFFFE && fmt.subFormatData1 == 0x00000001);
  const bool supportedDepth = (bitDepth == 16 || bitDepth == 24);
  if (!pcmFormat || fmt.numChannels != 2 || !supportedDepth || dataSize == 0 || dataPos == std::streampos(-1) || !sawFmt) {
    std::cerr << "Unsupported WAV format in: " << path << "\n";
    return buffer;
  }

  in.seekg(dataPos);

  buffer.sampleRate = static_cast<int>(fmt.sampleRate);
  const uint16_t bytesPerSample = static_cast<uint16_t>(fmt.bitsPerSample / 8);
  const size_t samples = dataSize / (fmt.numChannels * bytesPerSample);
  buffer.left.resize(samples);
  buffer.right.resize(samples);

  if (bitDepth == 16) {
    for (size_t i = 0; i < samples; ++i) {
      int16_t l = readLE16(in);
      int16_t r = readLE16(in);
      buffer.left[i] = static_cast<float>(l) * kInv32768;
      buffer.right[i] = static_cast<float>(r) * kInv32768;
    }
  } else {
    for (size_t i = 0; i < samples; ++i) {
      int32_t l = readLE24(in);
      int32_t r = readLE24(in);
      buffer.left[i] = static_cast<float>(l) * kInv8388608;
      buffer.right[i] = static_cast<float>(r) * kInv8388608;
    }
  }

  return buffer;
}

void writeStereoWav(const std::filesystem::path &path, const StereoBuffer &buffer) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  if (!out.good()) {
    std::cerr << "Could not write WAV: " << path << "\n";
    return;
  }

  const uint16_t bitsPerSample = 16;
  const uint16_t numChannels = 2;
  const uint32_t byteRate = buffer.sampleRate * numChannels * (bitsPerSample / 8);
  const uint16_t blockAlign = numChannels * (bitsPerSample / 8);
  const uint32_t dataSize = static_cast<uint32_t>(buffer.left.size() * blockAlign);

  out.write("RIFF", 4);
  writeLE32(out, 36 + dataSize);
  out.write("WAVE", 4);

  out.write("fmt ", 4);
  writeLE32(out, 16);
  writeLE16(out, 1);
  writeLE16(out, numChannels);
  writeLE32(out, static_cast<uint32_t>(buffer.sampleRate));
  writeLE32(out, byteRate);
  writeLE16(out, blockAlign);
  writeLE16(out, bitsPerSample);

  out.write("data", 4);
  writeLE32(out, dataSize);

  for (size_t i = 0; i < buffer.left.size(); ++i) {
    float l = std::clamp(buffer.left[i], -1.0f, 1.0f);
    float r = std::clamp(buffer.right[i], -1.0f, 1.0f);
    writeLE16(out, static_cast<int16_t>(std::lrintf(l * 32767.0f)));
    writeLE16(out, static_cast<int16_t>(std::lrintf(r * 32767.0f)));
  }
}

std::vector<WavRender> buildDemoRenders(const StereoBuffer &input) {
  std::vector<WavRender> renders;

  renders.push_back({"light",
                     {.width = 0.65f,
                      .dynWidth = 0.25f,
                      .transientSens = 0.35f,
                      .midTiltDbPerOct = 0.75f,
                      .sideAirFreqHz = 9000.0f,
                      .sideAirGainDb = 1.5f,
                      .lowAnchorHz = 140.0f,
                      .dirtAmount = 0.08f,
                      .ceilingDb = -1.5f,
                      .limiterReleaseMs = 120.0f,
                      .limiterLookaheadMs = 5.5f,
                      .limiterTiltDbPerOct = 0.0f,
                      .limiterLink = LimiterLookahead::LinkMode::Linked,
                      .limiterMix = 0.7f,
                      .limiterBypass = false,
                      .outputTrimDb = 0.5f},
                     input});

  renders.push_back({"mid",
                     {.width = 0.72f,
                      .dynWidth = 0.4f,
                      .transientSens = 0.5f,
                      .midTiltDbPerOct = 0.5f,
                      .sideAirFreqHz = 11000.0f,
                      .sideAirGainDb = 2.5f,
                      .lowAnchorHz = 120.0f,
                      .dirtAmount = 0.15f,
                      .ceilingDb = -2.5f,
                      .limiterReleaseMs = 95.0f,
                      .limiterLookaheadMs = 6.0f,
                      .limiterTiltDbPerOct = 0.5f,
                      .limiterLink = LimiterLookahead::LinkMode::Linked,
                      .limiterMix = 0.78f,
                      .limiterBypass = false,
                      .outputTrimDb = 0.0f},
                     input});

  renders.push_back({"heavy",
                     {.width = 0.85f,
                      .dynWidth = 0.6f,
                      .transientSens = 0.7f,
                      .midTiltDbPerOct = -0.5f,
                      .sideAirFreqHz = 8500.0f,
                      .sideAirGainDb = 1.0f,
                      .lowAnchorHz = 110.0f,
                      .dirtAmount = 0.25f,
                      .ceilingDb = -5.0f,
                      .limiterReleaseMs = 70.0f,
                      .limiterLookaheadMs = 6.5f,
                      .limiterTiltDbPerOct = 1.5f,
                      .limiterLink = LimiterLookahead::LinkMode::MidSide,
                      .limiterMix = 0.85f,
                      .limiterBypass = false,
                      .outputTrimDb = -1.5f},
                     input});

  renders.push_back({"kitchen_sink",
                     {.width = 0.95f,
                      .dynWidth = 0.75f,
                      .transientSens = 0.8f,
                      .midTiltDbPerOct = 1.5f,
                      .sideAirFreqHz = 14000.0f,
                      .sideAirGainDb = 4.0f,
                      .lowAnchorHz = 90.0f,
                      .dirtAmount = 0.35f,
                      .ceilingDb = -6.0f,
                      .limiterReleaseMs = 55.0f,
                      .limiterLookaheadMs = 7.5f,
                      .limiterTiltDbPerOct = 2.5f,
                      .limiterLink = LimiterLookahead::LinkMode::MidSide,
                      .limiterMix = 0.9f,
                      .limiterBypass = false,
                      .outputTrimDb = 1.0f},
                     input});

  renders.push_back({"tilt_air_extremes",
                     {.width = 0.7f,
                      .dynWidth = 0.35f,
                      .transientSens = 0.45f,
                      .midTiltDbPerOct = 6.0f,
                      .sideAirFreqHz = 16000.0f,
                      .sideAirGainDb = 5.5f,
                      .lowAnchorHz = 115.0f,
                      .dirtAmount = 0.12f,
                      .ceilingDb = -3.0f,
                      .limiterReleaseMs = 80.0f,
                      .limiterLookaheadMs = 5.0f,
                      .limiterTiltDbPerOct = 1.0f,
                      .limiterLink = LimiterLookahead::LinkMode::Linked,
                      .limiterMix = 0.9f,
                      .limiterBypass = false,
                      .outputTrimDb = -0.5f},
                     input});

  renders.push_back({"link_toggle_scope",
                     {.width = 0.82f,
                      .dynWidth = 0.5f,
                      .transientSens = 0.55f,
                      .midTiltDbPerOct = 0.25f,
                      .sideAirFreqHz = 12500.0f,
                      .sideAirGainDb = 2.5f,
                      .lowAnchorHz = 105.0f,
                      .dirtAmount = 0.2f,
                      .ceilingDb = -2.0f,
                      .limiterReleaseMs = 85.0f,
                      .limiterLookaheadMs = 6.5f,
                      .limiterTiltDbPerOct = 0.5f,
                      .limiterLink = LimiterLookahead::LinkMode::MidSide,
                      .limiterMix = 0.8f,
                      .limiterBypass = false,
                      .outputTrimDb = 0.0f},
                     input});

  return renders;
}

