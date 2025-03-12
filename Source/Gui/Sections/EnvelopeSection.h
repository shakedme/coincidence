#pragma once


#include <memory>

#include "BaseSection.h"
#include "../Components/EnvelopeComponent.h"

class EnvelopeSectionComponent : public BaseSectionComponent
{
public:
    EnvelopeSectionComponent(PluginEditor& editor, PluginProcessor& processor);
    ~EnvelopeSectionComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Get the envelope component for waveform visualization
    EnvelopeComponent* getEnvelopeComponent() { return envelopeComponent.get(); }

private:
    enum class Rate
    {
        TwoWhole = 0,
        Whole,
        Half,
        Quarter,     // 1/4 note - 1 beat
        Eighth,      // 1/8 note - 1/2 beat
        Sixteenth,   // 1/16 note - 1/4 beat
        ThirtySecond // 1/32 note - 1/8 beat
    };

    std::unique_ptr<juce::ComboBox> rateComboBox;
    std::unique_ptr<juce::Label> rateLabel;
    std::unique_ptr<juce::TabbedComponent> paramTabs;
    std::unique_ptr<EnvelopeComponent> envelopeComponent;
    std::unique_ptr<juce::TextButton> directionButton;
    std::unique_ptr<juce::Slider> scaleSlider;
    std::unique_ptr<juce::Label> scaleLabel;

    void setupRateComboBox();
    void updateTimeRangeFromRate();
    void setupDirectionButton();
    void setupScaleSlider();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeSectionComponent)
}; 