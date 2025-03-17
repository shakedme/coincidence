#pragma once

#include <memory>

#include "BaseSection.h"
#include "../Components/EnvelopeComponent.h"
#include "../Components/WaveformComponent.h"

class EnvelopeTabs : public juce::TabbedComponent {
public:
    explicit EnvelopeTabs(juce::TabbedButtonBar::Orientation orientation)
            : juce::TabbedComponent(orientation) {
        setInterceptsMouseClicks(false, true);
        setColour(juce::TabbedComponent::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::TabbedComponent::outlineColourId, juce::Colours::transparentBlack);
    }

    std::function<void(int tabIndex)> onTabChanged;

    void currentTabChanged(int newCurrentTabIndex, const juce::String & /*newCurrentTabName*/) override {
        if (onTabChanged)
            onTabChanged(newCurrentTabIndex);
    }
};


class EnvelopeSectionComponent : public BaseSectionComponent {
public:
    EnvelopeSectionComponent(PluginEditor &editor, PluginProcessor &processor);

    ~EnvelopeSectionComponent() override = default;

    void paint(juce::Graphics &g) override;

    void resized() override;

    // Get the envelope component for external access
    EnvelopeComponent *getEnvelopeComponent() { return envelopeComponent.get(); }

    // Get the reverb envelope component for external access
    EnvelopeComponent *getReverbEnvelopeComponent() { return reverbEnvelopeComponent.get(); }

    // Get the waveform component for external access
    WaveformComponent *getWaveformComponent() { return waveformComponent.get(); }

private:
    std::unique_ptr<EnvelopeTabs> envTabs;
    std::unique_ptr<EnvelopeComponent> envelopeComponent;
    std::unique_ptr<EnvelopeComponent> reverbEnvelopeComponent;
    std::unique_ptr<WaveformComponent> waveformComponent;
    std::unique_ptr<juce::Slider> scaleSlider;
    std::unique_ptr<juce::Label> scaleLabel;

    void setupScaleSlider();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeSectionComponent)
}; 