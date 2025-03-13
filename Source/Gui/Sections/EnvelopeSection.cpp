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
    paramTabs->addTab("EQ", juce::Colours::transparentBlack, new juce::Component(), true);
    paramTabs->addTab("Pitch", juce::Colours::transparentBlack, new juce::Component(), true);

    addAndMakeVisible(paramTabs.get());

    // Create the envelope component
    envelopeComponent = std::make_unique<EnvelopeComponent>(processor.getTimingManager());
    addAndMakeVisible(envelopeComponent.get());

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
        if (envelopeComponent != nullptr) {
            envelopeComponent->setWaveformScaleFactor(static_cast<float>(scaleSlider->getValue()));
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
    paramTabs->setBounds(area.removeFromTop(tabsHeight));

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

    // Position the envelope component in the remaining space
    envelopeComponent->setBounds(area.reduced(10, 10));
} 