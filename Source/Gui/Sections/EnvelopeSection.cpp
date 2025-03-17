#include "EnvelopeSection.h"

#include <memory>

EnvelopeSectionComponent::EnvelopeSectionComponent(PluginEditor &editor, PluginProcessor &processor)
        : BaseSectionComponent(editor, processor, "", juce::Colour(0xff8a6e9e)) {
    envTabs = std::make_unique<EnvelopeTabs>(juce::TabbedButtonBar::TabsAtTop);
    envTabs->setOutline(0);
    envTabs->setTabBarDepth(30);

    envTabs->addTab("Amplitude", juce::Colours::transparentBlack, new juce::Component(), true);
    envTabs->addTab("Reverb", juce::Colours::transparentBlack, new juce::Component(), true);

    addAndMakeVisible(envTabs.get());

    // Create the amplitude envelope component
    envelopeComponent = std::make_unique<EnvelopeComponent>(processor.getTimingManager(),
                                                            EnvelopeParams::ParameterType::Amplitude);
    addAndMakeVisible(envelopeComponent.get());

    // Create the reverb envelope component (initially hidden)
    reverbEnvelopeComponent = std::make_unique<EnvelopeComponent>(processor.getTimingManager(),
                                                                  EnvelopeParams::ParameterType::Reverb);
    addChildComponent(reverbEnvelopeComponent.get());

    // Show the active tab's envelope component
    envTabs->onTabChanged = [this](int tabIndex) {
        if (tabIndex == 0) { // Amplitude tab
            envelopeComponent->setVisible(true);
            reverbEnvelopeComponent->setVisible(false);
        } else if (tabIndex == 1) { // Reverb tab
            envelopeComponent->setVisible(false);
            reverbEnvelopeComponent->setVisible(true);
        } else { // Other tabs (EQ, Pitch)
            envelopeComponent->setVisible(false);
            reverbEnvelopeComponent->setVisible(false);
        }
    };

    // Set up scale slider
    setupScaleSlider();
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
        float scaleFactor = static_cast<float>(scaleSlider->getValue());
        
        // Apply scale to amplitude envelope
        if (envelopeComponent != nullptr) {
            envelopeComponent->setWaveformScaleFactor(scaleFactor);
        }
        
        // Apply same scale to reverb envelope
        if (reverbEnvelopeComponent != nullptr) {
            reverbEnvelopeComponent->setWaveformScaleFactor(scaleFactor);
        }
    };
    addAndMakeVisible(scaleSlider.get());
}

void EnvelopeSectionComponent::paint(juce::Graphics &g) {
    BaseSectionComponent::paint(g);
}

void EnvelopeSectionComponent::resized() {
    auto area = getLocalBounds();

    // Position the parameter tabs at the top
    const int tabsHeight = 30;
    envTabs->setBounds(area.removeFromTop(tabsHeight));

    // Position the controls at the bottom
    const int controlHeight = 25;
    const int bottomMargin = 10;
    auto controlArea = area.removeFromBottom(controlHeight + bottomMargin);

    // Position scale slider - we removed rate controls
    const int labelWidth = 40;
    const int sliderWidth = 80;
    const int spacing = 10;

    // Scale slider (now the only control)
    scaleLabel->setBounds(10, controlArea.getY(), labelWidth, controlHeight);
    scaleSlider->setBounds(scaleLabel->getRight(), controlArea.getY(), sliderWidth, controlHeight);

    // Position the envelope components in the remaining space
    auto envelopeArea = area.reduced(10, 10);
    envelopeComponent->setBounds(envelopeArea);
    reverbEnvelopeComponent->setBounds(envelopeArea);
} 