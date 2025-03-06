//
// Created by Shaked Melman on 01/03/2025.
//

#include "BaseSection.h"

BaseSectionComponent::BaseSectionComponent(PluginEditor& e,
                                           PluginProcessor& p,
                                           const juce::String& title,
                                           juce::Colour colour)
    : editor(e)
    , processor(p)
    , sectionTitle(title)
    , sectionColour(colour)
{
    sectionLabel = std::make_unique<juce::Label>();
    sectionLabel->setText(title, juce::dontSendNotification);
    sectionLabel->setJustificationType(juce::Justification::centred);
    sectionLabel->setFont(
        juce::Font(juce::FontOptions(TITLE_FONT_SIZE, juce::Font::bold)));
    sectionLabel->setColour(juce::Label::textColourId, colour);
    addAndMakeVisible(sectionLabel.get());
}

// Update the paint method to call drawMetallicPanel
void BaseSectionComponent::paint(juce::Graphics& g)
{
    // Draw the metallic panel first
    drawMetallicPanel(g);

    // Set the bounds for the section label consistently
    sectionLabel->setBounds(0, 5, getWidth(), HEADER_HEIGHT);

    // Draw the accent line under the section header
    g.setColour(sectionColour.withAlpha(0.5f));
    g.drawLine(10, HEADER_HEIGHT + 5, getWidth() - 10, HEADER_HEIGHT + 5, 1.0f);
}

juce::Label* BaseSectionComponent::createLabel(const juce::String& text,
                                               juce::Justification justification)
{
    auto* label = new juce::Label();
    label->setText(text, juce::dontSendNotification);
    label->setJustificationType(justification);
    label->setFont(juce::Font(juce::FontOptions(11.0f)));
    return label;
}

juce::Slider* BaseSectionComponent::createRotarySlider(const juce::String& tooltip)
{
    auto* slider =
        new juce::Slider(juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow);
    slider->setTooltip(tooltip);
    slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 12);
    slider->setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider->setColour(juce::Slider::textBoxBackgroundColourId,
                      juce::Colours::transparentBlack);
    slider->setColour(juce::Slider::textBoxOutlineColourId,
                      juce::Colours::transparentBlack);
    slider->setNumDecimalPlacesToDisplay(0);
    return slider;
}

void BaseSectionComponent::clearAttachments()
{
    sliderAttachments.clear();
    buttonAttachments.clear();
    comboBoxAttachments.clear();
}

void BaseSectionComponent::drawMetallicPanel(juce::Graphics& g)
{
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

    auto* lf = dynamic_cast<LookAndFeel*>(&getLookAndFeel());
    if (lf != nullptr)
    {
        lf->drawScrew(g, bounds.getX() + 10, bounds.getY() + 10, 8);
        lf->drawScrew(g, bounds.getRight() - 10, bounds.getY() + 10, 8);
        lf->drawScrew(g, bounds.getX() + 10, bounds.getBottom() - 10, 8);
        lf->drawScrew(g, bounds.getRight() - 10, bounds.getBottom() - 10, 8);
    }
}

void BaseSectionComponent::initKnob(std::unique_ptr<juce::Slider>& knob,
                                    const juce::String& tooltip,
                                    const juce::String& name,
                                    int min,
                                    int max,
                                    double interval,
                                    const juce::String& suffix)
{
    knob = std::unique_ptr<juce::Slider>(createRotarySlider(tooltip));
    knob->setName(name);
    knob->setRange(min, max, interval);
    knob->setTextValueSuffix("%");
    addAndMakeVisible(knob.get());
}

void BaseSectionComponent::initLabel(std::unique_ptr<juce::Label>& label, const juce::String& text, juce::Justification justification, float fontSize)
{
    label = std::unique_ptr<juce::Label>(createLabel(text, justification));
    label->setFont(juce::Font(juce::FontOptions(fontSize)));
    addAndMakeVisible(label.get());
}