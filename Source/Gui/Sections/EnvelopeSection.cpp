#include "EnvelopeSection.h"
#include "../Components/Envelope/EnvelopeComponent.h"
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
            auto component = std::make_shared<EnvelopeComponent>(
                    processor.getTimingManager(), typeInfo.type);

            // Initially all components are hidden
            addAndMakeVisible(component.get());
            component->setVisible(false);

            // Store the component
            envelopeComponents[typeInfo.type] = component;

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

std::weak_ptr<EnvelopeComponent> EnvelopeSectionComponent::getEnvelopeComponent(EnvelopeParams::ParameterType type) {
    auto it = envelopeComponents.find(type);
    if (it != envelopeComponents.end()) {
        return it->second;
    }
    return {};
}

void EnvelopeSectionComponent::paint(juce::Graphics &g) {
    BaseSectionComponent::paint(g);
}

void EnvelopeSectionComponent::resized() {
    auto area = getLocalBounds();

    // Position the parameter tabs at the top
    const int tabsHeight = 30;
    envTabs->setBounds(area.removeFromTop(tabsHeight));

    // Position all envelope components in the remaining space
    auto envelopeArea = area.reduced(10, 10);
    for (auto &pair: envelopeComponents) {
        pair.second->setBounds(envelopeArea);
    }
} 