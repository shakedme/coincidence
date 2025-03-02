//
// Created by Shaked Melman on 01/03/2025.
//

#include "GlitchSection.h"

GlitchSectionComponent::GlitchSectionComponent(MidiGeneratorEditor& editor,
                                               MidiGeneratorProcessor& processor)
    : BaseSectionComponent(editor, processor, "GLITCH", juce::Colour(0xffd9a652))
{
    // Create dummy knobs for the Glitch section
    const char* glitchNames[6] = {
        "CRUSH", "STUTTER", "CHAOS", "REVERSE", "JUMP", "GLIDE"};

    for (int i = 0; i < 6; ++i)
    {
        // Create glitch knob
        glitchKnobs[i] = std::unique_ptr<juce::Slider>(
            createRotarySlider(glitchNames[i] + juce::String(" intensity")));
        glitchKnobs[i]->setName("glitch_" + juce::String(i));
        glitchKnobs[i]->setRange(0.0, 100.0, 0.1);
        glitchKnobs[i]->setTextValueSuffix("%");
        addAndMakeVisible(glitchKnobs[i].get());

        // Create glitch label
        glitchLabels[i] = std::unique_ptr<juce::Label>(
            createLabel(glitchNames[i], juce::Justification::centred));
        glitchLabels[i]->setFont(juce::Font(11.0f, juce::Font::bold));
        addAndMakeVisible(glitchLabels[i].get());

        // These parameters don't exist yet, so we won't create attachments
    }
}

void GlitchSectionComponent::resized()
{
    auto area = getLocalBounds();

    // Set up header
    sectionLabel->setBounds(area.getX(), 35, area.getWidth(), 25); // Reduced from 30

    // Configure Glitch section - evenly distribute 6 knobs
    const int knobSize = 45; // Reduced from 60
    const int labelHeight = 18;
    const int numGlitchKnobs = 6;
    const int glitchKnobPadding =
        (area.getWidth() - (numGlitchKnobs * knobSize)) / (numGlitchKnobs + 1);
    const int glitchKnobY = 75;

    for (int i = 0; i < numGlitchKnobs; ++i)
    {
        int xPos = area.getX() + glitchKnobPadding + i * (knobSize + glitchKnobPadding);
        glitchKnobs[i]->setBounds(xPos, glitchKnobY, knobSize, knobSize);
        glitchLabels[i]->setBounds(xPos, glitchKnobY + knobSize, knobSize, labelHeight);
    }
}