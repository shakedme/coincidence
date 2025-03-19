#include "EnvelopeSection.h"
#include <memory>

EnvelopeSectionComponent::EnvelopeSectionComponent(PluginEditor &editor, PluginProcessor &processor)
        : BaseSectionComponent(editor, processor, "", juce::Colour(0xff8a6e9e)) {

    // Create ADSR component
    adsrComponent = std::make_unique<ADSREnvelopeComponent>(processor);
    addAndMakeVisible(adsrComponent.get());

    // Create LFO tabs and components
    lfoTabs = std::make_unique<EnvelopeTabs>(juce::TabbedButtonBar::TabsAtTop);
    lfoTabs->setOutline(0);
    lfoTabs->setTabBarDepth(30);
    addAndMakeVisible(lfoTabs.get());

    createLFOComponents();
}

void EnvelopeSectionComponent::createLFOComponents() {
    lfoTabs->addTab("LFO 1", juce::Colours::transparentBlack, nullptr, false);
    lfoTabs->addTab("LFO 2", juce::Colours::transparentBlack, nullptr, false);
    lfoTabs->addTab("LFO 3", juce::Colours::transparentBlack, nullptr, false);

    for (int i = 0; i < 3; ++i) {
        auto component = std::make_unique<EnvelopeComponent>(processor);
        addChildComponent(component.get());
        lfoComponents.insert(std::make_pair(i, std::move(component)));
    }

    lfoComponents[0]->setVisible(true);

    lfoTabs->onTabChanged = [this](int tabIndex) {
        for (auto &pair: lfoComponents) {
            pair.second->setVisible(false);
        }
        lfoComponents[tabIndex]->setVisible(true);
    };
}

void EnvelopeSectionComponent::paint(juce::Graphics &g) {
    BaseSectionComponent::paint(g);

    auto bounds = getLocalBounds().toFloat();

    // Draw a subtle divider line between ADSR and LFO sections
    float dividerX = bounds.getWidth() / 3.0f;
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawLine(dividerX, bounds.getY() + 30, dividerX, bounds.getBottom(), 1.0f);
}

void EnvelopeSectionComponent::resized() {
    auto area = getLocalBounds();

    // Divide the section: 1/3 for ADSR, 2/3 for LFO
    auto adsrArea = area.removeFromLeft(area.getWidth() / 3);
    auto lfoArea = area;

    // Position ADSR component
    adsrComponent->setBounds(adsrArea.reduced(10, 10));

    // Position LFO tabs and components
    const int tabsHeight = 30;
    lfoTabs->setBounds(lfoArea.removeFromTop(tabsHeight));

    auto lfoComponentArea = lfoArea.reduced(10, 10);
    for (auto &pair: lfoComponents) {
        pair.second->setBounds(lfoComponentArea);
    }
} 