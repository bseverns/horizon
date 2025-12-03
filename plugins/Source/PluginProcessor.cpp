#include "PluginProcessor.h"
#include "PluginEditor.h"

HorizonAudioProcessor::HorizonAudioProcessor()
  : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    _state(*this, nullptr, "HORIZON_STATE", createParameterLayout()) {}

void HorizonAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  _processor.prepareToPlay(sampleRate, samplesPerBlock);
}

bool HorizonAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) {
    return false;
  }
  if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo()) {
    return false;
  }
  return true;
}

void HorizonAudioProcessor::pushParametersToProcessor() {
  auto& state = _state;
  _processor.setWidth(*state.getRawParameterValue("width"));
  _processor.setDynWidth(*state.getRawParameterValue("dynWidth"));
  _processor.setTransientSens(*state.getRawParameterValue("transient"));
  _processor.setMidTilt(*state.getRawParameterValue("midTilt"));
  _processor.setSideAir(*state.getRawParameterValue("airFreq"),
                        *state.getRawParameterValue("airGain"));
  _processor.setLowAnchor(*state.getRawParameterValue("lowAnchor"));
  _processor.setDirt(*state.getRawParameterValue("dirt"));
  _processor.setCeiling(*state.getRawParameterValue("limitCeiling"));
  _processor.setLimiterReleaseMs(*state.getRawParameterValue("limitRelease"));
  _processor.setLimiterLookaheadMs(*state.getRawParameterValue("limitLook"));
  _processor.setLimiterDetectorTilt(*state.getRawParameterValue("limitTilt"));
  _processor.setLimiterMix(*state.getRawParameterValue("limitMix"));

  auto linkChoice = static_cast<int>(*state.getRawParameterValue("limitLink"));
  auto linkMode = (linkChoice == 0) ? LimiterLookahead::LinkMode::Linked
                                    : LimiterLookahead::LinkMode::MidSide;
  _processor.setLimiterLinkMode(linkMode);

  _processor.setMix(*state.getRawParameterValue("mix"));
  _processor.setOutputTrim(*state.getRawParameterValue("outTrim"));
}

void HorizonAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer&) {
  juce::ScopedNoDenormals noDenormals;

  if (buffer.getNumChannels() < 2) {
    buffer.setSize(2, buffer.getNumSamples(), true, true, true);
  }

  pushParametersToProcessor();

  auto numSamples = buffer.getNumSamples();
  auto* left = buffer.getWritePointer(0);
  auto* right = buffer.getWritePointer(1);

  _processor.processBlock(left, right, left, right, numSamples, getSampleRate());

  _telemetryWidth.store(_processor.getBlockWidth());
  _telemetryTransient.store(_processor.getBlockTransient());
  _telemetryLimiterGR.store(_processor.getLimiterGRdB());
  _telemetryLimiterClipped.store(_processor.getLimiterClipFlagAndClear());
}

void HorizonAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
  if (auto xml = _state.copyState().createXml()) {
    copyXmlToBinary(*xml, destData);
  }
}

void HorizonAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
  if (auto xmlState = getXmlFromBinary(data, sizeInBytes)) {
    _state.replaceState(juce::ValueTree::fromXml(*xmlState));
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout
HorizonAudioProcessor::createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  auto linear01 = [](const juce::String& id, const juce::String& name, float defVal) {
    return std::make_unique<juce::AudioParameterFloat>(id, name,
                                                       juce::NormalisableRange<float>(0.0f, 1.0f),
                                                       defVal);
  };

  params.push_back(linear01("width", "Width", 0.6f));
  params.push_back(linear01("dynWidth", "Dyn Width", 0.35f));
  params.push_back(linear01("transient", "Transient", 0.5f));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "midTilt", "Tilt", juce::NormalisableRange<float>(-6.0f, 6.0f), 0.0f));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "airFreq", "Air Freq",
    juce::NormalisableRange<float>(4000.0f, 16000.0f, 0.001f, 0.35f), 10000.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "airGain", "Air Gain", juce::NormalisableRange<float>(-6.0f, 6.0f), 2.0f));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "lowAnchor", "Low Anchor", juce::NormalisableRange<float>(40.0f, 250.0f, 0.01f, 0.4f),
    100.0f));

  params.push_back(linear01("dirt", "Dirt", 0.1f));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "limitCeiling", "Limit Ceiling",
    juce::NormalisableRange<float>(-12.0f, -0.1f), -1.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "limitRelease", "Limit Release", juce::NormalisableRange<float>(20.0f, 200.0f), 80.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "limitLook", "Lookahead", juce::NormalisableRange<float>(1.0f, 8.0f), 5.8f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "limitTilt", "Detector Tilt", juce::NormalisableRange<float>(-3.0f, 3.0f), 0.0f));
  params.push_back(linear01("limitMix", "Limit Mix", 0.7f));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
    "limitLink", "Limit Link", juce::StringArray{"Linked", "Mid/Side"}, 0));

  params.push_back(linear01("mix", "Mix", 0.6f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "outTrim", "Output Trim", juce::NormalisableRange<float>(-12.0f, 6.0f), 0.0f));

  return {params.begin(), params.end()};
}

juce::AudioProcessorEditor* HorizonAudioProcessor::createEditor() {
  return new HorizonAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new HorizonAudioProcessor();
}
