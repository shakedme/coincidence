//
// Created by Shaked Melman on 01/03/2025.
//

#include "PitchSection.h"
#include "../../Audio/MidiGeneratorParams.h"

PitchSectionComponent::PitchSectionComponent(MidiGeneratorEditor& editor,
                                             MidiGeneratorProcessor& processor)
    : BaseSectionComponent(editor, processor, "PITCH", juce::Colour(0xff52d97d))
{
    setupScaleTypeControls();
    setupSemitoneControls();
    setupOctaveControls();
}

void PitchSectionComponent::resized()
{
    auto area = getLocalBounds();

    // Set up header
    sectionLabel->setBounds(area.getX(), 5, area.getWidth(), 25); // Reduced from 30

    // Scale type dropdown at the top
    scaleTypeComboBox->setBounds(area.getCentreX() - 90, 35, 180, 25); // Reduced size
    scaleLabel->setBounds(area.getCentreX() - 40, 65, 80, 18); // Moved up

    // Semitone and Octave controls
    const int knobSize = 45; // Reduced from 60
    const int labelHeight = 18;
    const int twoKnobPadding = (area.getWidth() - (2 * knobSize)) / 3;

    // Middle row - Semitone controls
    const int middleRowY = 95; // Reduced from 120
    semitonesKnob->setBounds(area.getX() + twoKnobPadding, middleRowY, knobSize, knobSize);
    semitonesLabel->setBounds(area.getX() + twoKnobPadding, middleRowY + knobSize, knobSize, labelHeight);

    semitonesProbabilityKnob->setBounds(area.getX() + 2 * twoKnobPadding + knobSize, middleRowY, knobSize, knobSize);
    semitonesProbabilityLabel->setBounds(area.getX() + 2 * twoKnobPadding + knobSize, middleRowY + knobSize, knobSize, labelHeight);

    // Bottom row - Octave controls
    const int bottomRowY = middleRowY + knobSize + labelHeight + 15; // Reduced from 30
    octavesKnob->setBounds(area.getX() + twoKnobPadding, bottomRowY, knobSize, knobSize);
    octavesLabel->setBounds(area.getX() + twoKnobPadding, bottomRowY + knobSize, knobSize, labelHeight);

    octavesProbabilityKnob->setBounds(area.getX() + 2 * twoKnobPadding + knobSize, bottomRowY, knobSize, knobSize);
    octavesProbabilityLabel->setBounds(area.getX() + 2 * twoKnobPadding + knobSize, bottomRowY + knobSize, knobSize, labelHeight);
}

void PitchSectionComponent::setupScaleTypeControls()
{
    // Create scale type combo box
    scaleTypeComboBox = std::make_unique<juce::ComboBox>();
    scaleTypeComboBox->addItem("MAJOR", MidiGeneratorParams::SCALE_MAJOR + 1);
    scaleTypeComboBox->addItem("MINOR", MidiGeneratorParams::SCALE_MINOR + 1);
    scaleTypeComboBox->addItem("PENTATONIC", MidiGeneratorParams::SCALE_PENTATONIC + 1);
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
    octavesLabel = std::unique_ptr<juce::Label>(createLabel("SHIFT", juce::Justification::centred));
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