#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

#include "HostHorizonProcessor.h"

class HorizonAudioProcessor : public juce::AudioProcessor {
public:
  HorizonAudioProcessor();
  ~HorizonAudioProcessor() override = default;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override {}

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return "Horizon"; }

  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState& getValueTreeState() { return _state; }

  float getTelemetryWidth() const { return _telemetryWidth.load(); }
  float getTelemetryTransient() const { return _telemetryTransient.load(); }
  float getTelemetryLimiterGR() const { return _telemetryLimiterGR.load(); }
  bool getTelemetryLimiterClipped() const { return _telemetryLimiterClipped.load(); }

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
  void pushParametersToProcessor();

  HostHorizonProcessor _processor;
  juce::AudioProcessorValueTreeState _state;

  std::atomic<float> _telemetryWidth{0.0f};
  std::atomic<float> _telemetryTransient{0.0f};
  std::atomic<float> _telemetryLimiterGR{0.0f};
  std::atomic<bool> _telemetryLimiterClipped{false};
};
