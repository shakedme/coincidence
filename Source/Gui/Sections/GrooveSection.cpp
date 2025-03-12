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
            (area.getWidth() - (Params::NUM_RATE_OPTIONS * knobSize))
            / (Params::NUM_RATE_OPTIONS + 1);

    for (size_t i = 0; i < Params::NUM_RATE_OPTIONS; ++i) {
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

    densityKnob->setBounds(area.getCentreX() - knobSize / 2, middleRowY, knobSize, knobSize);
    densityLabel->setBounds(area.getCentreX() - knobSize / 2, middleRowY + knobSize, knobSize, labelHeight);

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
    for (size_t i = 0; i < Params::NUM_RATE_OPTIONS; ++i) {
        // Create rate knob
        rateKnobs[i] = std::unique_ptr<juce::Slider>(
                createRotarySlider("Rate " + juce::String(Params::rateBaseNames[i]) + " intensity"));
        rateKnobs[i]->setName("rate_" + juce::String(i));
        rateKnobs[i]->setRange(0.0, 100.0, 0.1);
        rateKnobs[i]->setTextValueSuffix("%");
        addAndMakeVisible(rateKnobs[i].get());

        // Create rate label
        rateLabels[i] = std::unique_ptr<juce::Label>(
                createLabel(Params::rateBaseNames[i], juce::Justification::centred));
        rateLabels[i]->setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        addAndMakeVisible(rateLabels[i].get());

        // Create parameter attachment
        sliderAttachments.push_back(
                std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                        processor.parameters,
                        "rate_" + juce::String(i) + "_value",
                        *rateKnobs[i]));
    }
}

void GrooveSectionComponent::setupRhythmModeControls() {
    // Create rhythm mode combo box
    rhythmModeComboBox = std::make_unique<juce::ComboBox>();
    rhythmModeComboBox->addItem("Normal", Params::RHYTHM_NORMAL + 1);
    rhythmModeComboBox->addItem("Dotted", Params::RHYTHM_DOTTED + 1);
    rhythmModeComboBox->addItem("Triplet", Params::RHYTHM_TRIPLET + 1);
    rhythmModeComboBox->setJustificationType(juce::Justification::centred);
    rhythmModeComboBox->setColour(juce::ComboBox::backgroundColourId,
                                  juce::Colour(0xff3a3a3a));
    rhythmModeComboBox->setColour(juce::ComboBox::textColourId, juce::Colours::white);
    rhythmModeComboBox->onChange = [this]() { updateRateLabelsForRhythmMode(); };
    addAndMakeVisible(rhythmModeComboBox.get());

    // Create rhythm mode label
    rhythmModeLabel =
            std::unique_ptr<juce::Label>(createLabel("MODE", juce::Justification::centred));
    rhythmModeLabel->setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    addAndMakeVisible(rhythmModeLabel.get());

    // Create parameter attachment
    comboBoxAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                    processor.parameters, "rhythm_mode", *rhythmModeComboBox));
}

void GrooveSectionComponent::setupDensityControls() {
    // Create density knob
    densityKnob =
            std::unique_ptr<juce::Slider>(createRotarySlider("Overall density/probability"));
    densityKnob->setName("density");
    densityKnob->setRange(0.0, 100.0, 0.1);
    densityKnob->setTextValueSuffix("%");
    addAndMakeVisible(densityKnob.get());

    // Create density label
    densityLabel = std::unique_ptr<juce::Label>(
            createLabel("DENSITY", juce::Justification::centred));
    densityLabel->setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    addAndMakeVisible(densityLabel.get());

    // Create parameter attachment
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.parameters, "density", *densityKnob));
}

void GrooveSectionComponent::setupGateControls() {
    // Create gate knob
    gateKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Gate length"));
    gateKnob->setName("gate");
    gateKnob->setRange(0.0, 100.0, 0.1);
    gateKnob->setTextValueSuffix("%");
    addAndMakeVisible(gateKnob.get());

    // Create gate label
    gateLabel =
            std::unique_ptr<juce::Label>(createLabel("GATE", juce::Justification::centred));
    gateLabel->setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    addAndMakeVisible(gateLabel.get());

    // Create gate random knob
    gateRandomKnob =
            std::unique_ptr<juce::Slider>(createRotarySlider("Gate randomization"));
    gateRandomKnob->setName("gate_random");
    gateRandomKnob->setRange(0.0, 100.0, 0.1);
    gateRandomKnob->setTextValueSuffix("%");
    addAndMakeVisible(gateRandomKnob.get());

    // Create gate random label
    gateRandomLabel =
            std::unique_ptr<juce::Label>(createLabel("RNDM", juce::Justification::centred));
    gateRandomLabel->setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    addAndMakeVisible(gateRandomLabel.get());

    // Create parameter attachments
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.parameters, "gate", *gateKnob));
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.parameters, "gate_randomize", *gateRandomKnob));
}

void GrooveSectionComponent::setupVelocityControls() {
    // Create velocity knob
    velocityKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Velocity"));
    velocityKnob->setName("velocity");
    velocityKnob->setRange(0.0, 100.0, 0.1);
    velocityKnob->setTextValueSuffix("%");
    addAndMakeVisible(velocityKnob.get());

    // Create velocity label
    velocityLabel =
            std::unique_ptr<juce::Label>(createLabel("VELO", juce::Justification::centred));
    velocityLabel->setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    addAndMakeVisible(velocityLabel.get());

    // Create velocity random knob
    velocityRandomKnob =
            std::unique_ptr<juce::Slider>(createRotarySlider("Velocity randomization"));
    velocityRandomKnob->setName("velocity_random");
    velocityRandomKnob->setRange(0.0, 100.0, 0.1);
    velocityRandomKnob->setTextValueSuffix("%");
    addAndMakeVisible(velocityRandomKnob.get());

    // Create velocity random label
    velocityRandomLabel =
            std::unique_ptr<juce::Label>(createLabel("RNDM", juce::Justification::centred));
    velocityRandomLabel->setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    addAndMakeVisible(velocityRandomLabel.get());

    // Create parameter attachments
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.parameters, "velocity", *velocityKnob));
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.parameters, "velocity_randomize", *velocityRandomKnob));
}

void GrooveSectionComponent::updateRateLabelsForRhythmMode() {
    // Don't proceed if the combo box isn't initialized yet
    if (rhythmModeComboBox == nullptr)
        return;

    // Get the current rhythm mode
    auto rhythmMode = static_cast<Params::RhythmMode>(
            rhythmModeComboBox->getSelectedItemIndex());

    // Get the rhythm mode text suffix
    juce::String rhythmSuffix;
    switch (rhythmMode) {
        case Params::RHYTHM_DOTTED:
            rhythmSuffix = "D";
            break;
        case Params::RHYTHM_TRIPLET:
            rhythmSuffix = "T";
            break;
        default:
            rhythmSuffix = "";
            break;
    }

    // Update the rate labels with the appropriate suffix

    for (int i = 0; i < Params::NUM_RATE_OPTIONS; ++i) {
        // Check if the label exists
        if (rateLabels[i] != nullptr) {
            juce::String labelText = Params::rateBaseNames[i];

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
            processor.parameters.getParameter("gate_direction"));

    if (gateDirectionParam)
        gateDirectionSelector->setDirection(static_cast<Params::DirectionType>(gateDirectionParam->getIndex()));

    gateDirectionSelector->onDirectionChanged = [this](Params::DirectionType direction) {
        auto *param = processor.parameters.getParameter("gate_direction");
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
            processor.parameters.getParameter("velocity_direction"));

    if (velocityDirectionParam)
        velocityDirectionSelector->setDirection(static_cast<Params::DirectionType>(velocityDirectionParam->getIndex()));

    velocityDirectionSelector->onDirectionChanged = [this](Params::DirectionType direction) {
        auto *param = processor.parameters.getParameter("velocity_direction");
        if (param)
            param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1(static_cast<int>(direction)));
        param->endChangeGesture();
    };
    addAndMakeVisible(velocityDirectionSelector.get());
}