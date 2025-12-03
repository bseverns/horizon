#include "PluginEditor.h"

namespace {
void makeMeterLabel(juce::Label& label, const juce::String& title) {
  label.setJustificationType(juce::Justification::centred);
  label.setText(title, juce::dontSendNotification);
  label.setColour(juce::Label::backgroundColourId, juce::Colours::black.withAlpha(0.2f));
  label.setColour(juce::Label::textColourId, juce::Colours::white);
  label.setFont(juce::Font(14.0f, juce::Font::bold));
}
}

SliderWithLabel::SliderWithLabel(const juce::String& labelText, juce::Slider::SliderStyle style)
  : slider(), label(labelText) {
  slider.setSliderStyle(style);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 60, 18);
  slider.setName(labelText);
  slider.setTooltip(labelText);
  addAndMakeVisible(slider);

  label.setJustificationType(juce::Justification::centred);
  label.setText(labelText, juce::dontSendNotification);
  label.setTooltip(labelText);
  addAndMakeVisible(label);
}

void SliderWithLabel::resized() {
  auto bounds = getLocalBounds();
  auto labelArea = bounds.removeFromTop(18);
  label.setBounds(labelArea);
  slider.setBounds(bounds.reduced(4));
}

HorizonAudioProcessorEditor::HorizonAudioProcessorEditor(HorizonAudioProcessor& processor)
  : AudioProcessorEditor(&processor),
    _processor(processor),
    _state(processor.getValueTreeState()),
    _width("Width"),
    _dynWidth("Dyn Width"),
    _transient("Transient"),
    _tilt("Tilt"),
    _airFreq("Air Freq"),
    _airGain("Air Gain"),
    _lowAnchor("Low Anchor"),
    _dirt("Dirt"),
    _limitCeiling("Ceiling"),
    _limitRelease("Release"),
    _limitLookahead("Lookahead"),
    _limitTilt("Tilt"),
    _limitMix("Limit Mix"),
    _mix("Mix"),
    _outTrim("Out Trim") {
  setSize(880, 520);

  auto rotary = juce::Slider::RotaryHorizontalVerticalDrag;
  _limitMix.slider.setSliderStyle(rotary);
  _mix.slider.setSliderStyle(rotary);

  auto& s = _state;
  _widthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, "width", _width.slider);
  _dynWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, "dynWidth", _dynWidth.slider);
  _transientAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, "transient", _transient.slider);
  _tiltAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, "midTilt", _tilt.slider);
  _airFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, "airFreq", _airFreq.slider);
  _airGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, "airGain", _airGain.slider);
  _lowAnchorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, "lowAnchor", _lowAnchor.slider);
  _dirtAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, "dirt", _dirt.slider);
  _limitCeilingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, "limitCeiling", _limitCeiling.slider);
  _limitReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, "limitRelease", _limitRelease.slider);
  _limitLookAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, "limitLook", _limitLookahead.slider);
  _limitTiltAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, "limitTilt", _limitTilt.slider);
  _limitMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, "limitMix", _limitMix.slider);
  _mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, "mix", _mix.slider);
  _outTrimAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, "outTrim", _outTrim.slider);

  _limitLink.addItem("Linked", 1);
  _limitLink.addItem("Mid/Side", 2);
  _limitLinkLabel.setText("Link", juce::dontSendNotification);
  _limitLinkLabel.setJustificationType(juce::Justification::centred);
  _limitLinkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(s, "limitLink", _limitLink);

  addAndMakeVisible(_width);
  addAndMakeVisible(_dynWidth);
  addAndMakeVisible(_transient);
  addAndMakeVisible(_tilt);
  addAndMakeVisible(_airFreq);
  addAndMakeVisible(_airGain);
  addAndMakeVisible(_lowAnchor);
  addAndMakeVisible(_dirt);
  addAndMakeVisible(_limitCeiling);
  addAndMakeVisible(_limitRelease);
  addAndMakeVisible(_limitLookahead);
  addAndMakeVisible(_limitTilt);
  addAndMakeVisible(_limitMix);
  addAndMakeVisible(_mix);
  addAndMakeVisible(_outTrim);
  addAndMakeVisible(_limitLink);
  addAndMakeVisible(_limitLinkLabel);

  makeMeterLabel(_meterWidth, "Width 0.00");
  makeMeterLabel(_meterTransient, "Transient 0.00");
  makeMeterLabel(_meterLimiter, "Limiter 0.0 dB");
  addAndMakeVisible(_meterWidth);
  addAndMakeVisible(_meterTransient);
  addAndMakeVisible(_meterLimiter);

  startTimerHz(30);
}

void HorizonAudioProcessorEditor::paint(juce::Graphics& g) {
  g.fillAll(juce::Colours::darkslategrey);
}

void HorizonAudioProcessorEditor::resized() {
  auto bounds = getLocalBounds().reduced(8);

  juce::Grid grid;
  grid.rowGap = juce::Grid::Px(6);
  grid.columnGap = juce::Grid::Px(6);
  grid.templateRows = { juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                        juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                        juce::Grid::TrackInfo(juce::Grid::Fr(1)) };
  grid.templateColumns = { juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                           juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                           juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                           juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                           juce::Grid::TrackInfo(juce::Grid::Fr(1)) };

  auto addItem = [&grid](juce::Component& comp) {
    grid.items.add(juce::GridItem(comp));
  };

  addItem(_width);
  addItem(_dynWidth);
  addItem(_transient);
  addItem(_tilt);
  addItem(_airFreq);
  addItem(_airGain);
  addItem(_lowAnchor);
  addItem(_dirt);
  addItem(_limitCeiling);
  addItem(_limitRelease);
  addItem(_limitLookahead);
  addItem(_limitTilt);
  addItem(_limitMix);
  addItem(_mix);
  addItem(_outTrim);

  auto gridArea = bounds.removeFromTop(bounds.getHeight() - 90);
  grid.performLayout(gridArea);

  auto meterArea = bounds.removeFromBottom(90);
  auto linkArea = meterArea.removeFromLeft(140);
  _limitLinkLabel.setBounds(linkArea.removeFromTop(20));
  _limitLink.setBounds(linkArea.reduced(4));

  auto meterWidthArea = meterArea.removeFromLeft(meterArea.getWidth() / 3);
  auto meterTransientArea = meterArea.removeFromLeft(meterArea.getWidth() / 2);
  _meterWidth.setBounds(meterWidthArea.reduced(4));
  _meterTransient.setBounds(meterTransientArea.reduced(4));
  _meterLimiter.setBounds(meterArea.reduced(4));
}

void HorizonAudioProcessorEditor::timerCallback() {
  auto width = _processor.getTelemetryWidth();
  auto transient = _processor.getTelemetryTransient();
  auto gr = _processor.getTelemetryLimiterGR();
  auto clipped = _processor.getTelemetryLimiterClipped();

  _meterWidth.setText("Width " + juce::String(width, 2), juce::dontSendNotification);
  _meterTransient.setText("Transient " + juce::String(transient, 2), juce::dontSendNotification);
  auto limiterText = juce::String("Limiter ") + juce::String(gr, 1) + " dB";
  if (clipped) {
    limiterText += " (clip)";
    _meterLimiter.setColour(juce::Label::backgroundColourId, juce::Colours::darkred.withAlpha(0.7f));
  } else {
    _meterLimiter.setColour(juce::Label::backgroundColourId, juce::Colours::black.withAlpha(0.2f));
  }
  _meterLimiter.setText(limiterText, juce::dontSendNotification);
}
