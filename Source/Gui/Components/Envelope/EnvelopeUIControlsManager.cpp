#include "EnvelopeUIControlsManager.h"

EnvelopeUIControlsManager::EnvelopeUIControlsManager()
        : currentRateEnum(Rate::Quarter),
          currentRate(1.0f),
          currentPresetShape(EnvelopePresetGenerator::PresetShape::Custom),
          snapToGridEnabled(true) {
}

void EnvelopeUIControlsManager::setupControls(juce::Component *parent) {
    // Setup all UI control groups
    setupRateUI(parent);
    setupPresetsUI(parent);
    setupSnapToGridUI(parent);
}

void EnvelopeUIControlsManager::setupRateUI(juce::Component *parent) {
    // Create rate label
    rateLabel = std::make_unique<juce::Label>("rateLabel", "Rate:");
    rateLabel->setFont(juce::Font(14.0f));
    rateLabel->setJustificationType(juce::Justification::centredRight);
    parent->addAndMakeVisible(rateLabel.get());

    // Create rate combo box
    rateComboBox = std::make_unique<juce::ComboBox>("rateComboBox");
    rateComboBox->addItem("2/1", static_cast<int>(Rate::TwoWhole) + 1);
    rateComboBox->addItem("1/1", static_cast<int>(Rate::Whole) + 1);
    rateComboBox->addItem("1/2", static_cast<int>(Rate::Half) + 1);
    rateComboBox->addItem("1/4", static_cast<int>(Rate::Quarter) + 1);
    rateComboBox->addItem("1/8", static_cast<int>(Rate::Eighth) + 1);
    rateComboBox->addItem("1/16", static_cast<int>(Rate::Sixteenth) + 1);
    rateComboBox->addItem("1/32", static_cast<int>(Rate::ThirtySecond) + 1);
    rateComboBox->setSelectedId(static_cast<int>(Rate::Quarter) + 1);
    rateComboBox->onChange = [this] {
        updateRateFromComboBox();
    };
    parent->addAndMakeVisible(rateComboBox.get());
}

void EnvelopeUIControlsManager::setupPresetsUI(juce::Component *parent) {
    // Create presets label
    presetShapesLabel = std::make_unique<juce::Label>("presetShapesLabel", "Shape:");
    presetShapesLabel->setFont(juce::Font(14.0f));
    presetShapesLabel->setJustificationType(juce::Justification::centredRight);
    parent->addAndMakeVisible(presetShapesLabel.get());

    // Create presets combo box
    presetShapesComboBox = std::make_unique<juce::ComboBox>("presetShapesComboBox");
    presetShapesComboBox->addItem("Sine", static_cast<int>(EnvelopePresetGenerator::PresetShape::Sine) + 1);
    presetShapesComboBox->addItem("Triangle", static_cast<int>(EnvelopePresetGenerator::PresetShape::Triangle) + 1);
    presetShapesComboBox->addItem("Square", static_cast<int>(EnvelopePresetGenerator::PresetShape::Square) + 1);
    presetShapesComboBox->addItem("Ramp Up", static_cast<int>(EnvelopePresetGenerator::PresetShape::RampUp) + 1);
    presetShapesComboBox->addItem("Ramp Down", static_cast<int>(EnvelopePresetGenerator::PresetShape::RampDown) + 1);
    presetShapesComboBox->addItem("Custom", static_cast<int>(EnvelopePresetGenerator::PresetShape::Custom) + 1);
    presetShapesComboBox->setSelectedId(static_cast<int>(EnvelopePresetGenerator::PresetShape::Custom) + 1);
    presetShapesComboBox->onChange = [this] {
        int selectedId = presetShapesComboBox->getSelectedId();
        if (selectedId > 0) {
            auto shape = static_cast<EnvelopePresetGenerator::PresetShape>(selectedId -
                                                                           1);
            setCurrentPresetShape(shape);
            if (onPresetShapeChanged) {
                onPresetShapeChanged(shape);
            }
        }
    };
    parent->addAndMakeVisible(presetShapesComboBox.get());
}

void EnvelopeUIControlsManager::setupSnapToGridUI(juce::Component *parent) {
    // Create snap-to-grid toggle button
    snapToGridButton = std::make_unique<juce::ToggleButton>("Snap to Grid");
    snapToGridButton->setToggleState(snapToGridEnabled, juce::dontSendNotification);
    snapToGridButton->onClick = [this] {
        snapToGridEnabled = snapToGridButton->getToggleState();
        if (onSnapToGridChanged) {
            onSnapToGridChanged(snapToGridEnabled);
        }
    };
    parent->addAndMakeVisible(snapToGridButton.get());
}

void EnvelopeUIControlsManager::resizeControls(int width, int topPadding) {
    // Calculate space for the rate controls
    const int controlHeight = 25;
    const int labelWidth = 40;
    const int comboWidth = 60;
    const int padding = topPadding;

    // Position rate controls at the top left
    rateLabel->setBounds(padding, padding, labelWidth, controlHeight);
    rateComboBox->setBounds(labelWidth + (2 * padding), padding, comboWidth, controlHeight);

    // Position preset shapes controls at the top right
    const int presetLabelWidth = 60;
    const int presetComboWidth = 90;
    presetShapesLabel->setBounds(width - presetLabelWidth - presetComboWidth - (2 * padding),
                                 padding, presetLabelWidth, controlHeight);
    presetShapesComboBox->setBounds(width - presetComboWidth - padding,
                                    padding, presetComboWidth, controlHeight);

    // Position snap-to-grid button at the center top
    const int snapButtonWidth = 100;
    snapToGridButton->setBounds((width - snapButtonWidth) / 2, padding, snapButtonWidth, controlHeight);
}

void EnvelopeUIControlsManager::updateRateFromComboBox() {
    // Update the current rate enum
    currentRateEnum = static_cast<Rate>(rateComboBox->getSelectedId() - 1);

    // Calculate and set the appropriate rate based on selection
    float newRate = 1.0f;
    switch (currentRateEnum) {
        case Rate::TwoWhole:
            newRate = 0.125f;
            break;
        case Rate::Whole:
            newRate = 0.25f;
            break;
        case Rate::Half:
            newRate = 0.5f;
            break;
        case Rate::Quarter:
            newRate = 1.0f;
            break;
        case Rate::Eighth:
            newRate = 2.0f;
            break;
        case Rate::Sixteenth:
            newRate = 4.0f;
            break;
        case Rate::ThirtySecond:
            newRate = 8.0f;
            break;
    }

    // Set the rate
    setRate(newRate);
}

void EnvelopeUIControlsManager::setRate(float rate) {
    currentRate = rate;

    // Notify listeners of rate change if callback is set
    if (onRateChanged) {
        onRateChanged(currentRate);
    }
}

void EnvelopeUIControlsManager::setCurrentPresetShape(EnvelopePresetGenerator::PresetShape shape) {
    currentPresetShape = shape;
    presetShapesComboBox->setSelectedId(static_cast<int>(shape) + 1, juce::dontSendNotification);
}

void EnvelopeUIControlsManager::setSnapToGridEnabled(bool enabled) {
    snapToGridEnabled = enabled;
    snapToGridButton->setToggleState(enabled, juce::dontSendNotification);

    if (onSnapToGridChanged) {
        onSnapToGridChanged(enabled);
    }
}

float EnvelopeUIControlsManager::calculateTimeRangeInSeconds(double bpm) const {
    const double beatsPerSecond = bpm / 60.0;

    // Calculate time range based on selected rate - this should match exactly one envelope cycle
    float timeRangeInSeconds = 1.0f;

    switch (currentRateEnum) {
        case Rate::TwoWhole:
            timeRangeInSeconds = static_cast<float>(8.0 / beatsPerSecond);
            break;
        case Rate::Whole:
            // One whole note = 4 quarter notes
            timeRangeInSeconds = static_cast<float>(4.0 / beatsPerSecond);
            break;
        case Rate::Half:
            // One half note = 2 quarter notes
            timeRangeInSeconds = static_cast<float>(2.0 / beatsPerSecond);
            break;
        case Rate::Quarter:
            // One quarter note
            timeRangeInSeconds = static_cast<float>(1.0 / beatsPerSecond);
            break;
        case Rate::Eighth:
            // One eighth note = 0.5 quarter notes
            timeRangeInSeconds = static_cast<float>(0.5 / beatsPerSecond);
            break;
        case Rate::Sixteenth:
            // One sixteenth note = 0.25 quarter notes
            timeRangeInSeconds = static_cast<float>(0.25 / beatsPerSecond);
            break;
        case Rate::ThirtySecond:
            // One thirty-second note = 0.125 quarter notes
            timeRangeInSeconds = static_cast<float>(0.125 / beatsPerSecond);
            break;
    }

    return timeRangeInSeconds;
} 