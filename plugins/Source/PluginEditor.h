#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class SliderWithLabel : public juce::Component {
public:
  SliderWithLabel(const juce::String& labelText, juce::Slider::SliderStyle style = juce::Slider::RotaryHorizontalVerticalDrag);

  void resized() override;

  juce::Slider slider;
  juce::Label label;
};

class HorizonAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer {
public:
  explicit HorizonAudioProcessorEditor(HorizonAudioProcessor& processor);
  ~HorizonAudioProcessorEditor() override = default;

  void paint(juce::Graphics& g) override;
  void resized() override;

private:
  void timerCallback() override;

  HorizonAudioProcessor& _processor;
  juce::AudioProcessorValueTreeState& _state;

  SliderWithLabel _width;
  SliderWithLabel _dynWidth;
  SliderWithLabel _transient;
  SliderWithLabel _tilt;
  SliderWithLabel _airFreq;
  SliderWithLabel _airGain;
  SliderWithLabel _lowAnchor;
  SliderWithLabel _dirt;
  SliderWithLabel _limitCeiling;
  SliderWithLabel _limitRelease;
  SliderWithLabel _limitLookahead;
  SliderWithLabel _limitTilt;
  SliderWithLabel _limitMix;
  SliderWithLabel _mix;
  SliderWithLabel _outTrim;

  juce::ComboBox _limitLink;
  juce::Label _limitLinkLabel;

  juce::Label _meterWidth;
  juce::Label _meterTransient;
  juce::Label _meterLimiter;

  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> _widthAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> _dynWidthAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> _transientAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> _tiltAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> _airFreqAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> _airGainAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> _lowAnchorAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> _dirtAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> _limitCeilingAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> _limitReleaseAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> _limitLookAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> _limitTiltAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> _limitMixAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> _mixAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> _outTrimAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> _limitLinkAttachment;
};
