//
// Created by Shaked Melman on 01/03/2025.
//

#include "BaseSection.h"

BaseSectionComponent::BaseSectionComponent(MidiGeneratorEditor& e,
                                           MidiGeneratorProcessor& p,
                                           const juce::String& title,
                                           juce::Colour colour)
    : editor(e)
    , processor(p)
    , sectionTitle(title)
    , sectionColour(colour)
{
    // Create section header
    sectionLabel = std::unique_ptr<juce::Label>(createLabel(title, juce::Justification::centred));
    sectionLabel->setFont(juce::Font(20.0f, juce::Font::bold));
    sectionLabel->setColour(juce::Label::textColourId, colour);
    addAndMakeVisible(sectionLabel.get());
}

void BaseSectionComponent::paint(juce::Graphics& g)
{
    // Default implementation - sections can override this
}

juce::Label* BaseSectionComponent::createLabel(const juce::String& text, juce::Justification justification)
{
    auto* label = new juce::Label();
    label->setText(text, juce::dontSendNotification);
    label->setJustificationType(justification);
    label->setFont(juce::Font(11.0f));
    return label;
}

juce::Slider* BaseSectionComponent::createRotarySlider(const juce::String& tooltip)
{
    auto* slider = new juce::Slider(juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow);
    slider->setTooltip(tooltip);
    slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 12);
    slider->setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider->setNumDecimalPlacesToDisplay(0);
    return slider;
}