#include "EnvelopeSection.h"
#include "../../Audio/Envelope/EnvelopeParameterTypes.h"
#include "../PluginEditor.h"
#include "../../Audio/PluginProcessor.h"
#include "../Components/EnvelopeComponent.h"
#include <memory>

EnvelopeSectionComponent::EnvelopeSectionComponent(PluginEditor &editor, PluginProcessor &processor)
        : BaseSectionComponent(editor, processor, "", juce::Colour(0xff8a6e9e)) {

    // Setup the tabbed interface
    envTabs = std::make_unique<EnvelopeTabs>(juce::TabbedButtonBar::TabsAtTop);
    envTabs->setOutline(0);
    envTabs->setTabBarDepth(30);
    addAndMakeVisible(envTabs.get());

    // Create envelope components for all registered types
    createEnvelopeComponents();

    // Set up scale slider for waveform visualization
    setupScaleSlider();

    // Show the first envelope component by default
    if (!envelopeComponents.empty()) {
        auto &typeInfo = processor.getEnvelopeRegistry().getAvailableTypes().front();
        envelopeComponents[typeInfo.type]->setVisible(true);
    }
}

void EnvelopeSectionComponent::createEnvelopeComponents() {
    // Get all available envelope types
    const auto &availableTypes = processor.getEnvelopeRegistry().getAvailableTypes();

    // Create envelope component and tab for each visible type
    int tabIndex = 0;
    for (const auto &typeInfo: availableTypes) {
        if (typeInfo.visible) {
            // Add tab
            envTabs->addTab(typeInfo.name, juce::Colours::transparentBlack, new juce::Component(), true);

            // Create envelope component
            auto component = std::make_unique<EnvelopeComponent>(
                    processor.getTimingManager(), typeInfo.type);

            // Initially all components are hidden
            component->setVisible(false);
            addAndMakeVisible(component.get());

            // Store the component
            envelopeComponents[typeInfo.type] = std::move(component);

            tabIndex++;
        }
    }

    // Setup tab change callback
    envTabs->onTabChanged = [this](int tabIndex) {
        // Hide all components
        for (auto &pair: envelopeComponents) {
            pair.second->setVisible(false);
        }

        // Show the selected component
        if (tabIndex >= 0 && tabIndex < processor.getEnvelopeRegistry().getAvailableTypes().size()) {
            const auto &typeInfo = processor.getEnvelopeRegistry().getAvailableTypes()[tabIndex];
            if (envelopeComponents.find(typeInfo.type) != envelopeComponents.end()) {
                envelopeComponents[typeInfo.type]->setVisible(true);
            }
        }
    };
}

EnvelopeComponent *EnvelopeSectionComponent::getEnvelopeComponent(EnvelopeParams::ParameterType type) {
    auto it = envelopeComponents.find(type);
    if (it != envelopeComponents.end()) {
        return it->second.get();
    }
    return nullptr;
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

        // Apply scale to all envelope components
        for (auto &pair: envelopeComponents) {
            if (pair.second != nullptr) {
                pair.second->setWaveformScaleFactor(scaleFactor);
            }
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

    // Position all envelope components in the remaining space
    auto envelopeArea = area.reduced(10, 10);
    for (auto &pair: envelopeComponents) {
        pair.second->setBounds(envelopeArea);
    }
} 