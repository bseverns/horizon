#include <portaudio.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "host/HostHorizonProcessor.h"

namespace {

struct PaHorizonContext {
  HostHorizonProcessor processor;
  double sampleRate = 48000.0;
  unsigned long blockSize = 0;
  std::vector<float> inL;
  std::vector<float> inR;
  std::vector<float> outL;
  std::vector<float> outR;
  std::atomic<bool> clipFlag{false};
};

int audio_callback(const void* inputBuffer,
                   void* outputBuffer,
                   unsigned long framesPerBuffer,
                   const PaStreamCallbackTimeInfo*,
                   PaStreamCallbackFlags,
                   void* userData) {
  auto* ctx          = static_cast<PaHorizonContext*>(userData);
  const float* input = static_cast<const float*>(inputBuffer);
  float* output      = static_cast<float*>(outputBuffer);

  if (!ctx || !output) {
    return paContinue;
  }

  if (framesPerBuffer != ctx->blockSize) {
    return paAbort;
  }

  std::fill(ctx->inL.begin(), ctx->inL.end(), 0.0f);
  std::fill(ctx->inR.begin(), ctx->inR.end(), 0.0f);
  std::fill(ctx->outL.begin(), ctx->outL.end(), 0.0f);
  std::fill(ctx->outR.begin(), ctx->outR.end(), 0.0f);

  if (input) {
    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
      ctx->inL[i] = input[2 * i + 0];
      ctx->inR[i] = input[2 * i + 1];
    }
  }

  ctx->processor.processBlock(ctx->inL.data(), ctx->inR.data(), ctx->outL.data(), ctx->outR.data(),
                              static_cast<int>(framesPerBuffer), ctx->sampleRate);

  for (unsigned long i = 0; i < framesPerBuffer; ++i) {
    output[2 * i + 0] = ctx->outL[i];
    output[2 * i + 1] = ctx->outR[i];
  }

  if (ctx->processor.getLimiterClipFlagAndClear()) {
    ctx->clipFlag.store(true);
  }

  return paContinue;
}

}  // namespace

int main() {
  constexpr unsigned long kBlockSize = 256;
  constexpr double kSampleRate       = 48000.0;

  PaError err = Pa_Initialize();
  if (err != paNoError) {
    std::cerr << "[horizon-pa] PortAudio init failed: " << Pa_GetErrorText(err) << "\n";
    return 1;
  }

  PaStreamParameters inputParams{};
  inputParams.device                    = Pa_GetDefaultInputDevice();
  if (inputParams.device == paNoDevice) {
    std::cerr << "[horizon-pa] No default input device.\n";
    Pa_Terminate();
    return 1;
  }
  inputParams.channelCount              = 2;
  inputParams.sampleFormat              = paFloat32;
  inputParams.suggestedLatency          = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
  inputParams.hostApiSpecificStreamInfo = nullptr;

  PaStreamParameters outputParams{};
  outputParams.device                    = Pa_GetDefaultOutputDevice();
  if (outputParams.device == paNoDevice) {
    std::cerr << "[horizon-pa] No default output device.\n";
    Pa_Terminate();
    return 1;
  }
  outputParams.channelCount              = 2;
  outputParams.sampleFormat              = paFloat32;
  outputParams.suggestedLatency          = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
  outputParams.hostApiSpecificStreamInfo = nullptr;

  PaHorizonContext ctx;
  ctx.sampleRate = kSampleRate;
  ctx.blockSize  = kBlockSize;
  ctx.inL.assign(kBlockSize, 0.0f);
  ctx.inR.assign(kBlockSize, 0.0f);
  ctx.outL.assign(kBlockSize, 0.0f);
  ctx.outR.assign(kBlockSize, 0.0f);

  ctx.processor.prepareToPlay(kSampleRate, static_cast<int>(kBlockSize));
  ctx.processor.setWidth(0.68f);
  ctx.processor.setDynWidth(0.45f);
  ctx.processor.setTransientSens(0.35f);
  ctx.processor.setSideAir(9500.0f, 1.5f);
  ctx.processor.setLowAnchor(140.0f);
  ctx.processor.setCeiling(-1.5f);
  ctx.processor.setMix(0.82f);

  PaStream* stream = nullptr;
  err              = Pa_OpenStream(&stream, &inputParams, &outputParams, kSampleRate, kBlockSize, paNoFlag,
                                   audio_callback, &ctx);
  if (err != paNoError) {
    std::cerr << "[horizon-pa] Failed to open stream: " << Pa_GetErrorText(err) << "\n";
    Pa_Terminate();
    return 1;
  }

  std::cout << "[horizon-pa] Live stereo demo. Speak or play audio; CTRL+C to exit.\n";
  err = Pa_StartStream(stream);
  if (err != paNoError) {
    std::cerr << "[horizon-pa] Failed to start stream: " << Pa_GetErrorText(err) << "\n";
    Pa_CloseStream(stream);
    Pa_Terminate();
    return 1;
  }

  while (Pa_IsStreamActive(stream) == 1) {
    Pa_Sleep(200);
    if (ctx.clipFlag.exchange(false)) {
      std::cout << "[horizon-pa] Limiter hit a clip guard. Consider pulling Ceiling or Trim.\n";
    }
  }

  Pa_StopStream(stream);
  Pa_CloseStream(stream);
  Pa_Terminate();
  return 0;
}
