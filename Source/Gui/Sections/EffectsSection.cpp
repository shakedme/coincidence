//
// Created by Shaked Melman on 01/03/2025.
//

#include "EffectsSection.h"

EffectsSection::EffectsSection(PluginEditor& e, PluginProcessor& p)
    : BaseSectionComponent(e, p, "EFFECTS", juce::Colour(0xffd9a652))
{
    stutterKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Stutter"));
    stutterKnob->setName("stutter_probability");
    stutterKnob->setRange(0.0, 100.0, 0.1);
    stutterKnob->setTextValueSuffix("%");
    addAndMakeVisible(stutterKnob.get());

    stutterLabel = std::unique_ptr<juce::Label>(
        createLabel("STUTTER", juce::Justification::centred));
    stutterLabel->setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    addAndMakeVisible(stutterLabel.get());

    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "glitch_stutter", *stutterKnob));
}

EffectsSection::~EffectsSection()
{
    clearAttachments();
}

void EffectsSection::resized()
{
//    auto area = getLocalBounds();

    const int knobSize = 45;
    const int labelHeight = 18;
    const int glitchKnobY = firstRowY;

    stutterKnob->setBounds(50, glitchKnobY, knobSize, knobSize);
    stutterLabel->setBounds(50, glitchKnobY + knobSize, knobSize, labelHeight);
}