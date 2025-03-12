#pragma once

#include <memory>

#include "BaseSection.h"
#include "../Components/EnvelopeComponent.h"
#include "../Components/WaveformComponent.h"

class EnvelopeSectionComponent : public BaseSectionComponent
{
public:
    EnvelopeSectionComponent(PluginEditor& editor, PluginProcessor& processor);
    ~EnvelopeSectionComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Get the envelope component for external access
    EnvelopeComponent* getEnvelopeComponent() { return envelopeComponent.get(); }

    // Get the waveform component for external access
    WaveformComponent* getWaveformComponent() { return waveformComponent.get(); }

private:
    std::unique_ptr<juce::TabbedComponent> paramTabs;
    std::unique_ptr<EnvelopeComponent> envelopeComponent;
    std::unique_ptr<WaveformComponent> waveformComponent;
    std::unique_ptr<juce::Slider> scaleSlider;
    std::unique_ptr<juce::Label> scaleLabel;

    void setupScaleSlider();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeSectionComponent)
}; 