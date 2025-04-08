//
// Created by Shaked Melman on 01/03/2025.
//

#include "GrooveSection.h"

GrooveSectionComponent::GrooveSectionComponent(PluginEditor &e,
                                               PluginProcessor &p)
        : BaseSectionComponent(e, p, "GROOVE", juce::Colour(0xff52bfd9)) {
    setupRateControls();
    setupRhythmModeControls();
    setupDensityControls();
    setupGateControls();
    setupVelocityControls();
    setupDirectionControls();
    updateRateLabelsForRhythmMode();
}

GrooveSectionComponent::~GrooveSectionComponent() {
    clearAttachments();
}

void GrooveSectionComponent::resized() {
    auto area = getLocalBounds();
    const int knobSize = 45; // Reduced from 60
    const int labelHeight = 18; // Reduced from 20

    // Top row - Rate knobs
    const int rateKnobY = firstRowY;
    const int knobPadding =
            (area.getWidth() - (Models::NUM_RATE_OPTIONS * knobSize))
            / (Models::NUM_RATE_OPTIONS + 1);

    for (size_t i = 0; i < Models::NUM_RATE_OPTIONS; ++i) {
        int xPos = area.getX() + knobPadding + i * (knobSize + knobPadding);
        rateKnobs[i]->setBounds(xPos, rateKnobY, knobSize, knobSize);
        rateLabels[i]->setBounds(xPos, rateKnobY + knobSize, knobSize, labelHeight);
    }

    // Middle row
    const int middleRowY = rateKnobY + knobSize + labelHeight + 25; // Reduced from 30

    const int gateGroupX = 30;
    const int velocityGroupX = area.getWidth() - 140;

    // Gate knobs
    gateKnob->setBounds(gateGroupX, middleRowY, knobSize, knobSize);
    gateLabel->setBounds(gateGroupX, middleRowY + knobSize, knobSize, labelHeight);

    gateRandomKnob->setBounds(gateGroupX + knobSize + 20, middleRowY, knobSize, knobSize);
    gateRandomLabel->setBounds(gateGroupX + knobSize + 20, middleRowY + knobSize, knobSize, labelHeight);

    gateDirectionSelector->setBounds(gateGroupX + 20,
                                     middleRowY + knobSize + labelHeight + 5,
                                     knobSize + 30,
                                     25);

    // Velocity knobs
    velocityKnob->setBounds(velocityGroupX, middleRowY, knobSize, knobSize);
    velocityLabel->setBounds(velocityGroupX, middleRowY + knobSize, knobSize, labelHeight);

    velocityRandomKnob->setBounds(velocityGroupX + knobSize + 20, middleRowY, knobSize, knobSize);
    velocityRandomLabel->setBounds(velocityGroupX + knobSize + 20, middleRowY + knobSize, knobSize, labelHeight);

    velocityDirectionSelector->setBounds(velocityGroupX + 20,
                                         middleRowY + knobSize + labelHeight + 5,
                                         knobSize + 30,
                                         25);

    probabilityKnob->setBounds(area.getCentreX() - 65 / 2, middleRowY, 65, 65);
    probabilityLabel->setBounds(area.getCentreX() - 65 / 2, middleRowY + 50, 65, 65);

    const int rhythmComboWidth = 90;
    const int rhythmComboHeight = 25;
    rhythmModeComboBox->setBounds(area.getCentreX() - 45,
                                  middleRowY + knobSize + labelHeight + 10,
                                  rhythmComboWidth,
                                  rhythmComboHeight);
    rhythmModeLabel->setBounds(rhythmModeComboBox->getX(),
                               middleRowY + knobSize + labelHeight + rhythmComboHeight + 10,
                               rhythmComboWidth,
                               labelHeight);
}

void GrooveSectionComponent::setupRateControls() {
    for (size_t i = 0; i < Models::NUM_RATE_OPTIONS; ++i) {
        auto rateName = Models::rateBaseNames[i];
        initKnob(rateKnobs[i], "Rate " + juce::String(Models::rateBaseNames[i]) + " intensity",
                 rateName);
        addAndMakeVisible(rateKnobs[i].get());

        // Create rate label
        initLabel(rateLabels[i], Models::rateBaseNames[i], juce::Justification::centred);
        addAndMakeVisible(rateLabels[i].get());

        // Create parameter attachment
        sliderAttachments.push_back(
                std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                        processor.getAPVTS(),
                        rateName,
                        *rateKnobs[i]));
    }
}

void GrooveSectionComponent::setupRhythmModeControls() {
    // Create rhythm mode combo box
    rhythmModeComboBox = std::make_unique<juce::ComboBox>();
    rhythmModeComboBox->addItem("Normal", Models::RHYTHM_NORMAL + 1);
    rhythmModeComboBox->addItem("Dotted", Models::RHYTHM_DOTTED + 1);
    rhythmModeComboBox->addItem("Triplet", Models::RHYTHM_TRIPLET + 1);
    rhythmModeComboBox->setJustificationType(juce::Justification::centred);
    rhythmModeComboBox->setColour(juce::ComboBox::backgroundColourId,
                                  juce::Colour(0xff3a3a3a));
    rhythmModeComboBox->setColour(juce::ComboBox::textColourId, juce::Colours::white);
    rhythmModeComboBox->onChange = [this]() { updateRateLabelsForRhythmMode(); };
//    addAndMakeVisible(rhythmModeComboBox.get());

    // Create rhythm mode label
    rhythmModeLabel =
            std::unique_ptr<juce::Label>(createLabel("MODE", juce::Justification::centred));
    rhythmModeLabel->setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
//    addAndMakeVisible(rhythmModeLabel.get());

    // Create parameter attachment
//    comboBoxAttachments.push_back(
//            std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
//                    processor.getAPVTS(), Params::ID_RHYTHM_MODE, *rhythmModeComboBox));
}

void GrooveSectionComponent::setupDensityControls() {
    // Create density knob
    initKnob(probabilityKnob, "Overall probability to play a note", "probability");
    addAndMakeVisible(probabilityKnob.get());

    // Create density label
    initLabel(probabilityLabel, "PROBABILITY", juce::Justification::centred, 14.0f);
    addAndMakeVisible(probabilityLabel.get());

    // Create parameter attachment
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_PROBABILITY, *probabilityKnob));
}

void GrooveSectionComponent::setupGateControls() {
    // Create gate knob
    initKnob(gateKnob, "Gate length", "gate");
    addAndMakeVisible(gateKnob.get());

    // Create gate label
    initLabel(gateLabel, "GATE", juce::Justification::centred);
    addAndMakeVisible(gateLabel.get());

    initKnob(gateRandomKnob, "Gate randomization", "gate_random");
    addAndMakeVisible(gateRandomKnob.get());

    // Create gate random label
    initLabel(gateRandomLabel, "RNDM", juce::Justification::centred);
    addAndMakeVisible(gateRandomLabel.get());

    // Create parameter attachments
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_GATE, *gateKnob));
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_GATE_RANDOMIZE, *gateRandomKnob));
}

void GrooveSectionComponent::setupVelocityControls() {
    // Create velocity knob
    initKnob(velocityKnob, "Velocity", "velocity");
    addAndMakeVisible(velocityKnob.get());

    // Create velocity label
    initLabel(velocityLabel, "VELO", juce::Justification::centred);
    addAndMakeVisible(velocityLabel.get());

    // Create velocity random knob
    initKnob(velocityRandomKnob, "Velocity randomization", "velocity_random");
    addAndMakeVisible(velocityRandomKnob.get());

    // Create velocity random label
    initLabel(velocityRandomLabel, "RNDM", juce::Justification::centred);
    addAndMakeVisible(velocityRandomLabel.get());

    // Create parameter attachments
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_VELOCITY, *velocityKnob));
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_VELOCITY_RANDOMIZE, *velocityRandomKnob));
}

void GrooveSectionComponent::updateRateLabelsForRhythmMode() {
    // Don't proceed if the combo box isn't initialized yet
    if (rhythmModeComboBox == nullptr)
        return;

    // Get the current rhythm mode
    auto rhythmMode = static_cast<Models::RhythmMode>(
            rhythmModeComboBox->getSelectedItemIndex());

    // Get the rhythm mode text suffix
    juce::String rhythmSuffix;
    switch (rhythmMode) {
        case Models::RHYTHM_DOTTED:
            rhythmSuffix = "D";
            break;
        case Models::RHYTHM_TRIPLET:
            rhythmSuffix = "T";
            break;
        default:
            rhythmSuffix = "";
            break;
    }

    // Update the rate labels with the appropriate suffix

    for (int i = 0; i < Models::NUM_RATE_OPTIONS; ++i) {
        // Check if the label exists
        if (rateLabels[i] != nullptr) {
            juce::String labelText = Models::rateBaseNames[i];

            // Only append suffix if it's not empty (i.e., not NORMAL mode)
            if (rhythmSuffix.isNotEmpty())
                labelText += rhythmSuffix;

            rateLabels[i]->setText(labelText, juce::dontSendNotification);
        }
    }
}

void GrooveSectionComponent::repaintRandomizationControls() {
    // Repaint knobs with randomization
    if (gateKnob != nullptr)
        gateKnob->repaint();
    if (velocityKnob != nullptr)
        velocityKnob->repaint();
}

void GrooveSectionComponent::setupDirectionControls() {
    // Create gate direction selector
    gateDirectionSelector = std::make_unique<DirectionSelector>(juce::Colour(0xff52bfd9));

    // Set initial value from parameter
    auto *gateDirectionParam = dynamic_cast<juce::AudioParameterChoice *>(
            processor.getAPVTS().getParameter(Params::ID_GATE_DIRECTION));

    if (gateDirectionParam)
        gateDirectionSelector->setDirection(static_cast<Models::DirectionType>(gateDirectionParam->getIndex()));

    gateDirectionSelector->onDirectionChanged = [this](Models::DirectionType direction) {
        auto *param = processor.getAPVTS().getParameter(Params::ID_GATE_DIRECTION);
        if (param)
            param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1(static_cast<int>(direction)));
        param->endChangeGesture();
    };

    addAndMakeVisible(gateDirectionSelector.get());

    // Create velocity direction selector
    velocityDirectionSelector =
            std::make_unique<DirectionSelector>(juce::Colour(0xff52bfd9));

    auto *velocityDirectionParam = dynamic_cast<juce::AudioParameterChoice *>(
            processor.getAPVTS().getParameter(Params::ID_VELOCITY_DIRECTION));

    if (velocityDirectionParam)
        velocityDirectionSelector->setDirection(static_cast<Models::DirectionType>(velocityDirectionParam->getIndex()));

    velocityDirectionSelector->onDirectionChanged = [this](Models::DirectionType direction) {
        auto *param = processor.getAPVTS().getParameter(Params::ID_VELOCITY_DIRECTION);
        if (param)
            param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1(static_cast<int>(direction)));
        param->endChangeGesture();
    };
    addAndMakeVisible(velocityDirectionSelector.get());
}