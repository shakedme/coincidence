#include "EnvelopeSection.h"

#include <memory>

EnvelopeSectionComponent::EnvelopeSectionComponent(PluginEditor &editor, PluginProcessor &processor)
        : BaseSectionComponent(editor, processor, "Envelope", juce::Colour(0xff8a6e9e)) {
    // Set up parameter tabs
    paramTabs = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);
    paramTabs->setOutline(0);
    paramTabs->setTabBarDepth(30);

    // Add empty content components for now - we'll implement the actual parameter tabs later
    paramTabs->addTab("Amplitude", juce::Colours::transparentBlack, new juce::Component(), true);
    paramTabs->addTab("Filter", juce::Colours::transparentBlack, new juce::Component(), true);
    paramTabs->addTab("Pitch", juce::Colours::transparentBlack, new juce::Component(), true);

    addAndMakeVisible(paramTabs.get());

    // Create the envelope component
    envelopeComponent = std::make_unique<EnvelopeComponent>();
    addAndMakeVisible(envelopeComponent.get());

    // Set up rate combo box
    setupRateComboBox();

    // Set up scale slider
    setupScaleSlider();

    // Initialize with default time range
    updateTimeRangeFromRate();
}

void EnvelopeSectionComponent::setupRateComboBox() {
    rateLabel = std::make_unique<juce::Label>("rateLabel", "Rate:");
    rateLabel->setFont(juce::Font(14.0f));
    rateLabel->setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(rateLabel.get());

    rateComboBox = std::make_unique<juce::ComboBox>("rateComboBox");
    rateComboBox->addItem("2/1", static_cast<int>(Rate::TwoWhole) + 1);
    rateComboBox->addItem("1/1", static_cast<int>(Rate::Whole) + 1);
    rateComboBox->addItem("1/2", static_cast<int>(Rate::Half) + 1);
    rateComboBox->addItem("1/4", static_cast<int>(Rate::Quarter) + 1);
    rateComboBox->addItem("1/8", static_cast<int>(Rate::Eighth) + 1);
    rateComboBox->addItem("1/16", static_cast<int>(Rate::Sixteenth) + 1);
    rateComboBox->addItem("1/32", static_cast<int>(Rate::ThirtySecond) + 1);
    rateComboBox->setSelectedId(static_cast<int>(Rate::Whole) + 1);
    rateComboBox->onChange = [this] {
        updateTimeRangeFromRate();
    };
    addAndMakeVisible(rateComboBox.get());
}

void EnvelopeSectionComponent::setupScaleSlider() {
    scaleLabel = std::make_unique<juce::Label>("scaleLabel", "Zoom");
    scaleLabel->setFont(juce::Font(14.0f));
    scaleLabel->setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(scaleLabel.get());

    scaleSlider = std::make_unique<juce::Slider>("scaleSlider");
    scaleSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    scaleSlider->setRange(0.1, 5.0, 0.1);
    scaleSlider->setValue(0.5);
    scaleSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    scaleSlider->setTooltip("Adjust waveform vertical scale");
    scaleSlider->onValueChange = [this] {
        if (envelopeComponent != nullptr) {
            envelopeComponent->setWaveformScaleFactor(static_cast<float>(scaleSlider->getValue()));
        }
    };
    addAndMakeVisible(scaleSlider.get());
}

void EnvelopeSectionComponent::updateTimeRangeFromRate() {
    if (envelopeComponent == nullptr || rateComboBox == nullptr)
        return;

    const double bpm = processor.getTimingManager().getBpm();
    const double beatsPerSecond = bpm / 60.0;

    // Calculate time range based on selected rate
    float timeRangeInSeconds = 1.0f;

    switch (static_cast<Rate>(rateComboBox->getSelectedId() - 1)) {
        case Rate::TwoWhole:
            // Four beats
            timeRangeInSeconds = static_cast<float>(8.0 / beatsPerSecond);
            break;
        case Rate::Whole:
            // Two beats
            timeRangeInSeconds = static_cast<float>(4.0 / beatsPerSecond);
            break;
        case Rate::Half:
            // Two beats
            timeRangeInSeconds = static_cast<float>(2.0 / beatsPerSecond);
            break;
        case Rate::Quarter:
            // One beat at 120 BPM = 0.5 seconds
            timeRangeInSeconds = static_cast<float>(1.0 / beatsPerSecond);
            break;

        case Rate::Eighth:
            // Half a beat
            timeRangeInSeconds = static_cast<float>(0.5 / beatsPerSecond);
            break;

        case Rate::Sixteenth:
            // Quarter beat
            timeRangeInSeconds = static_cast<float>(0.25 / beatsPerSecond);
            break;

        case Rate::ThirtySecond:
            // Eighth beat
            timeRangeInSeconds = static_cast<float>(0.125 / beatsPerSecond);
            break;
    }

    // Update the envelope component
    envelopeComponent->setTimeRange(timeRangeInSeconds);
}

void EnvelopeSectionComponent::paint(juce::Graphics &g) {
    BaseSectionComponent::paint(g);
}

void EnvelopeSectionComponent::resized() {
    auto area = getLocalBounds();

    // Position the parameter tabs at the top
    const int tabsHeight = 30;
    paramTabs->setBounds(area.removeFromTop(tabsHeight));

    // Position the controls at the bottom
    const int controlHeight = 25;
    const int bottomMargin = 10;
    auto controlArea = area.removeFromBottom(controlHeight + bottomMargin);

    // Position rate controls
    const int labelWidth = 40;
    const int comboWidth = 60;
    const int spacing = 10;

    // Rate controls
    rateLabel->setBounds(10, controlArea.getY(), labelWidth, controlHeight);
    rateComboBox->setBounds(rateLabel->getRight(), controlArea.getY(), comboWidth, controlHeight);

    // Scale slider
    const int sliderWidth = 80;
    scaleLabel->setBounds(rateComboBox->getRight() + spacing, controlArea.getY(), labelWidth, controlHeight);
    scaleSlider->setBounds(scaleLabel->getRight(), controlArea.getY(), sliderWidth, controlHeight);

    // Position the envelope component in the remaining space
    envelopeComponent->setBounds(area.reduced(10, 10));
} 