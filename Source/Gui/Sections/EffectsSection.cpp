//
// Created by Shaked Melman on 01/03/2025.
//

#include "EffectsSection.h"

EffectsSection::EffectsSection(PluginEditor& e, PluginProcessor& p)
    : BaseSectionComponent(e, p, "EFFECTS", juce::Colour(0xffd9a652))
{
    // Stutter effect control
    initKnob(stutterKnob, "Stutter", "stutter_probability", 0, 100, 0.1, "%");
    initLabel(stutterLabel, "STUTTER");

    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "glitch_stutter", *stutterKnob));

    // Reverb section label
    reverbSectionLabel =
        std::unique_ptr<juce::Label>(createLabel("REVERB", juce::Justification::centred));
    reverbSectionLabel->setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    reverbSectionLabel->setColour(juce::Label::textColourId,
                                  sectionColour.withAlpha(0.8f));
    addAndMakeVisible(reverbSectionLabel.get());

    // Reverb controls

    // Mix knob
    initKnob(reverbMixKnob, "Reverb Mix", "reverb_mix", 0, 100, 0.1, "%");
    initLabel(reverbMixLabel, "MIX");

    // Probability knob
    initKnob(reverbProbabilityKnob,
             "Reverb Probability",
             "reverb_probability",
             0,
             100,
             0.1,
             "%");
    initLabel(reverbProbabilityLabel, "PROBABILITY");

    // Time knob
    initKnob(reverbTimeKnob, "Reverb Time", "reverb_time", 0, 100, 0.1, "%");
    initLabel(reverbTimeLabel, "TIME");

    // Damping knob
    initKnob(reverbDampingKnob, "Reverb Damping", "reverb_damping", 0, 100, 0.1, "%");
    initLabel(reverbDampingLabel, "DAMPING");

    // Width knob
    initKnob(reverbWidthKnob, "Reverb Width", "reverb_width", 0, 100, 0.1, "%");
    initLabel(reverbWidthLabel, "WIDTH");

    // Parameter attachments
    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "reverb_mix", *reverbMixKnob));

    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "reverb_probability", *reverbProbabilityKnob));

    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "reverb_time", *reverbTimeKnob));

    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "reverb_damping", *reverbDampingKnob));

    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "reverb_width", *reverbWidthKnob));

}

EffectsSection::~EffectsSection()
{
    clearAttachments();
}

void EffectsSection::paint(juce::Graphics& g)
{
    // Call the base class paint method
    BaseSectionComponent::paint(g);

    auto area = getLocalBounds();

    const int knobSize = 45;
    const int rowY = firstRowY + knobSize / 2;

    // First vertical line - after stutter, before reverb
    const int divider1X = area.getWidth() * 0.25f;
    g.setColour(sectionColour.withAlpha(0.3f));
    g.drawLine(divider1X, rowY - 30, divider1X, rowY + 60, 1.0f);

    // Second vertical line - after reverb section
    const int divider2X = area.getWidth() * 0.75f;
    g.setColour(sectionColour.withAlpha(0.3f));
    g.drawLine(divider2X, rowY - 30, divider2X, rowY + 60, 1.0f);

    // Draw the reverb label above the reverb controls
    g.setColour(sectionColour.withAlpha(0.8f));
    g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    g.drawText("REVERB",
               divider1X + 10,
               rowY - 30,
               divider2X - divider1X - 20,
               20,
               juce::Justification::centred);
}

void EffectsSection::resized()
{
    auto area = getLocalBounds();

    const int knobSize = 45;
    const int labelHeight = 18;
    const int rowY = firstRowY + 20;

    // Calculate positions for all knobs in one row
    const int stutterX = area.getWidth() * 0.125f - knobSize / 2;
    const int reverbMixX = area.getWidth() * 0.375f - knobSize / 2;
    const int reverbTimeX = area.getWidth() * 0.5f - knobSize / 2;
    const int reverbProbX = area.getWidth() * 0.625f - knobSize / 2;

    // Position stutter knob
    stutterKnob->setBounds(stutterX, rowY, knobSize, knobSize);
    stutterLabel->setBounds(stutterX, rowY + knobSize, knobSize, labelHeight);

    const int divider1X = area.getWidth() * 0.25f;
    const int divider2X = area.getWidth() * 0.75f;
    const int reverbSectionWidth = divider2X - divider1X;
    const int reverbKnobGap = reverbSectionWidth - 10 / 5 * knobSize;
    // Hide the reverb section label as we're now drawing it directly in paint()
    reverbSectionLabel->setVisible(false);

    // Position reverb knobs
    reverbMixKnob->setBounds(reverbMixX, rowY, knobSize, knobSize);
    reverbMixLabel->setBounds(reverbMixX, rowY + knobSize, knobSize, labelHeight);

    reverbTimeKnob->setBounds(reverbTimeX + reverbKnobGap, rowY, knobSize, knobSize);
    reverbTimeLabel->setBounds(
        reverbTimeX + reverbKnobGap, rowY + knobSize, knobSize, labelHeight);

    reverbProbabilityKnob->setBounds(reverbProbX + 2 * reverbKnobGap, rowY, knobSize, knobSize);
    reverbProbabilityLabel->setBounds(
        reverbProbX - 10, rowY + knobSize, knobSize + 20, labelHeight);

    reverbDampingKnob->setBounds(reverbProbX+ 2 * reverbKnobGap, rowY, knobSize, knobSize);
    reverbDampingLabel->setBounds(
        reverbProbX - 10, rowY + knobSize, knobSize + 20, labelHeight);

    reverbWidthKnob->setBounds(reverbProbX+ 2 * reverbKnobGap, rowY, knobSize, knobSize);
    reverbWidthLabel->setBounds(
        reverbProbX - 10, rowY + knobSize, knobSize + 20, labelHeight);
}