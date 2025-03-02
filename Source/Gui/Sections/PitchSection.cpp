//
// Created by Shaked Melman on 01/03/2025.
//

#include "PitchSection.h"
#include "../../Audio/Params.h"

PitchSectionComponent::PitchSectionComponent(PluginEditor& editor,
                                             PluginProcessor& processor)
    : BaseSectionComponent(editor, processor, "PITCH", juce::Colour(0xff52d97d))
{
    setupScaleTypeControls();
    setupSemitoneControls();
    setupOctaveControls();
}

PitchSectionComponent::~PitchSectionComponent()
{
    clearAttachments();
}

void PitchSectionComponent::paint(juce::Graphics& g)
{
    BaseSectionComponent::paint(g);

    auto bounds = getLocalBounds();
    g.drawLine(getWidth() / 2, 100, getWidth() / 2, bounds.getHeight() - 10, 1.0f);
}

void PitchSectionComponent::resized()
{
    auto area = getLocalBounds();

    // Scale type dropdown at the top - center it
    const int comboBoxWidth = juce::jmin(180, area.getWidth() - 20);
    scaleTypeComboBox->setBounds(area.getCentreX() - comboBoxWidth/2, firstRowY, comboBoxWidth, 25);
    scaleLabel->setBounds(area.getCentreX() - 40, 75, 80, 18);

    // Semitone and Octave controls - make them stack vertically instead of side by side
    const int knobSize = 45;
    const int labelHeight = 18;
    const int quarterWidth = area.getWidth() / 4;
    const int leftColumnX = quarterWidth - knobSize / 2;
    const int rightColumnX = quarterWidth * 3 - knobSize / 2;
    const int knobPadding = 15;

    const int firstRowY = 90;
    const int secondRowY = firstRowY + knobSize + labelHeight + knobPadding;

    // First row
    semitonesKnob->setBounds(leftColumnX, firstRowY, knobSize, knobSize);
    semitonesLabel->setBounds(leftColumnX, firstRowY + knobSize, knobSize, labelHeight);
    octavesKnob->setBounds(rightColumnX, firstRowY, knobSize, knobSize);
    octavesLabel->setBounds(rightColumnX, firstRowY + knobSize, knobSize, labelHeight);

    // Second row
    semitonesProbabilityKnob->setBounds(leftColumnX, secondRowY, knobSize, knobSize);
    semitonesProbabilityLabel->setBounds(leftColumnX, secondRowY + knobSize, knobSize, labelHeight);
    octavesProbabilityKnob->setBounds(rightColumnX, secondRowY, knobSize, knobSize);
    octavesProbabilityLabel->setBounds(rightColumnX, secondRowY + knobSize, knobSize, labelHeight);
}

void PitchSectionComponent::setupScaleTypeControls()
{
    // Create scale type combo box
    scaleTypeComboBox = std::make_unique<juce::ComboBox>();
    scaleTypeComboBox->addItem("MAJOR", Params::SCALE_MAJOR + 1);
    scaleTypeComboBox->addItem("MINOR", Params::SCALE_MINOR + 1);
    scaleTypeComboBox->addItem("PENTATONIC", Params::SCALE_PENTATONIC + 1);
    scaleTypeComboBox->setJustificationType(juce::Justification::centred);
    scaleTypeComboBox->setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff3a3a3a));
    scaleTypeComboBox->setColour(juce::ComboBox::textColourId, juce::Colours::white);
    addAndMakeVisible(scaleTypeComboBox.get());

    // Create scale label
    scaleLabel = std::unique_ptr<juce::Label>(createLabel("SCALE", juce::Justification::centred));
    scaleLabel->setFont(juce::Font(11.0f, juce::Font::bold));
    addAndMakeVisible(scaleLabel.get());

    // Create parameter attachment
    comboBoxAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.parameters, "scale_type", *scaleTypeComboBox));
}

void PitchSectionComponent::setupSemitoneControls()
{
    // Create semitones knob
    semitonesKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Semitone range"));
    semitonesKnob->setName("semitones");
    semitonesKnob->setRange(0, 12, 1);
    addAndMakeVisible(semitonesKnob.get());

    // Create semitones label
    semitonesLabel = std::unique_ptr<juce::Label>(createLabel("STEPS", juce::Justification::centred));
    semitonesLabel->setFont(juce::Font(11.0f, juce::Font::bold));
    addAndMakeVisible(semitonesLabel.get());

    // Create semitones probability knob
    semitonesProbabilityKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Semitone variation probability"));
    semitonesProbabilityKnob->setName("semitones_prob");
    semitonesProbabilityKnob->setRange(0.0, 100.0, 0.1);
    semitonesProbabilityKnob->setTextValueSuffix("%");
    addAndMakeVisible(semitonesProbabilityKnob.get());

    // Create semitones probability label
    semitonesProbabilityLabel = std::unique_ptr<juce::Label>(createLabel("CHANCE", juce::Justification::centred));
    semitonesProbabilityLabel->setFont(juce::Font(11.0f, juce::Font::bold));
    addAndMakeVisible(semitonesProbabilityLabel.get());

    // Create parameter attachments
    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "semitones", *semitonesKnob));
    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "semitones_prob", *semitonesProbabilityKnob));
}

void PitchSectionComponent::setupOctaveControls()
{
    // Create octaves knob
    octavesKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Octave range"));
    octavesKnob->setName("octaves");
    octavesKnob->setRange(0, 3, 1);
    addAndMakeVisible(octavesKnob.get());

    // Create octaves label
    octavesLabel = std::unique_ptr<juce::Label>(createLabel("OCTAVE", juce::Justification::centred));
    octavesLabel->setFont(juce::Font(11.0f, juce::Font::bold));
    addAndMakeVisible(octavesLabel.get());

    // Create octaves probability knob
    octavesProbabilityKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Octave variation probability"));
    octavesProbabilityKnob->setName("octaves_prob");
    octavesProbabilityKnob->setRange(0.0, 100.0, 0.1);
    octavesProbabilityKnob->setTextValueSuffix("%");
    addAndMakeVisible(octavesProbabilityKnob.get());

    // Create octaves probability label
    octavesProbabilityLabel = std::unique_ptr<juce::Label>(createLabel("CHANCE", juce::Justification::centred));
    octavesProbabilityLabel->setFont(juce::Font(11.0f, juce::Font::bold));
    addAndMakeVisible(octavesProbabilityLabel.get());

    // Create parameter attachments
    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "octaves", *octavesKnob));
    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "octaves_prob", *octavesProbabilityKnob));
}