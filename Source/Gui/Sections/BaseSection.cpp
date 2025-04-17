//
// Created by Shaked Melman on 01/03/2025.
//

#include "BaseSection.h"
#include "../Components/KnobComponent.h"

BaseSectionComponent::BaseSectionComponent(PluginEditor &e,
                                           PluginProcessor &p,
                                           const juce::String &title,
                                           juce::Colour colour)
        : editor(e), processor(p), sectionTitle(title), sectionColour(colour) {
    sectionLabel = std::make_unique<juce::Label>();
    sectionLabel->setText(title, juce::dontSendNotification);
    sectionLabel->setJustificationType(juce::Justification::centred);
    sectionLabel->setFont(
            juce::Font(juce::FontOptions(TITLE_FONT_SIZE, juce::Font::bold)));
    sectionLabel->setColour(juce::Label::textColourId, colour);
    addAndMakeVisible(sectionLabel.get());
}

void BaseSectionComponent::paint(juce::Graphics &g) {
    drawMetallicPanel(g);

    sectionLabel->setBounds(0, 5, getWidth(), HEADER_HEIGHT);

    g.setColour(sectionColour.withAlpha(0.5f));
    g.drawLine(10, HEADER_HEIGHT + 5, getWidth() - 10, HEADER_HEIGHT + 5, 1.0f);
}

juce::Label *BaseSectionComponent::createLabel(const juce::String &text,
                                               juce::Justification justification) {
    auto *label = new juce::Label();
    label->setText(text, juce::dontSendNotification);
    label->setJustificationType(justification);
    label->setFont(juce::Font(juce::FontOptions(11.0f)));
    return label;
}

void BaseSectionComponent::clearAttachments() {
    sliderAttachments.clear();
    buttonAttachments.clear();
    comboBoxAttachments.clear();
}

void BaseSectionComponent::drawMetallicPanel(juce::Graphics &g) {
    auto bounds = getLocalBounds();
    auto baseColor = juce::Colour(0xff2a2a2a);

    // Draw the panel background
    g.setGradientFill(juce::ColourGradient(baseColor.brighter(0.1f),
                                           bounds.getX(),
                                           bounds.getY(),
                                           baseColor.darker(0.1f),
                                           bounds.getX(),
                                           bounds.getBottom(),
                                           false));
    g.fillRect(bounds);

    // Draw a subtle inner shadow
    g.setColour(juce::Colour(0x20000000));
    g.drawRect(bounds.expanded(1, 1), 2);

    // Draw a highlight on the top edge
    g.setColour(juce::Colour(0x30ffffff));
    g.drawLine(bounds.getX() + 2,
               bounds.getY() + 1,
               bounds.getRight() - 2,
               bounds.getY() + 1,
               1.0f);
}

void BaseSectionComponent::initKnob(std::unique_ptr<juce::Slider> &knob,
                                    const juce::String &tooltip,
                                    const juce::String &name,
                                    int min,
                                    int max,
                                    double interval,
                                    const juce::String &suffix) {
    knob = std::unique_ptr<juce::Slider>(new KnobComponent(processor.getModulationMatrix(), tooltip));
    knob->setName(name);
    knob->setRange(min, max, interval);
    knob->setTextValueSuffix("%");
    addAndMakeVisible(knob.get());
}

void BaseSectionComponent::initLabel(std::unique_ptr<juce::Label> &label, const juce::String &text,
                                     juce::Justification justification, float fontSize) {
    label = std::unique_ptr<juce::Label>(createLabel(text, justification));
    label->setFont(juce::Font(juce::FontOptions(fontSize)));
    addAndMakeVisible(label.get());
}