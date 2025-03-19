#include "EnvelopeSection.h"
#include "../Components/Envelope/EnvelopeComponent.h"
#include <memory>

EnvelopeSectionComponent::EnvelopeSectionComponent(PluginEditor &editor, PluginProcessor &processor)
        : BaseSectionComponent(editor, processor, "", juce::Colour(0xff8a6e9e)) {

    envTabs = std::make_unique<EnvelopeTabs>(juce::TabbedButtonBar::TabsAtTop);
    envTabs->setOutline(0);
    envTabs->setTabBarDepth(30);
    addAndMakeVisible(envTabs.get());

    createEnvelopeComponents();
}

void EnvelopeSectionComponent::createEnvelopeComponents() {
    for (const auto &typeInfo: EnvelopeParams::getAvailableTypes()) {
        envTabs->addTab(typeInfo.name, juce::Colours::transparentBlack, nullptr, false);
        auto component = std::make_unique<EnvelopeComponent>(processor, typeInfo.id);
        addAndMakeVisible(component.get());
        component->setVisible(false);
        envelopeComponents[typeInfo.type] = std::move(component);
    }

    envelopeComponents[EnvelopeParams::ParameterType::Amplitude]->setVisible(true);

    envTabs->onTabChanged = [this](int tabIndex) {
        for (auto &pair: envelopeComponents) {
            pair.second->setVisible(false);
        }
        const auto typeInfo = EnvelopeParams::getAvailableTypes()[tabIndex];
        envelopeComponents[typeInfo.type]->setVisible(true);
    };
}

void EnvelopeSectionComponent::paint(juce::Graphics &g) {
    BaseSectionComponent::paint(g);
}

void EnvelopeSectionComponent::resized() {
    auto area = getLocalBounds();

    const int tabsHeight = 30;
    envTabs->setBounds(area.removeFromTop(tabsHeight));

    auto envelopeArea = area.reduced(10, 10);
    for (auto &pair: envelopeComponents) {
        pair.second->setBounds(envelopeArea);
    }
} 