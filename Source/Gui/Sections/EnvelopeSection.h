#pragma once

#include "../Components/Envelope/EnvelopeTabs.h"
#include "BaseSection.h"
#include <map>
#include "../Components/Envelope/EnvelopeParameterTypes.h"

// Forward declarations
class PluginEditor;

class PluginProcessor;

class EnvelopeComponent;

class EnvelopeSectionComponent : public BaseSectionComponent {
public:
    EnvelopeSectionComponent(PluginEditor &editor, PluginProcessor &processor);

    ~EnvelopeSectionComponent() override = default;

    void paint(juce::Graphics &g) override;

    void resized() override;

    void createEnvelopeComponents();

private:
    std::unique_ptr<EnvelopeTabs> envTabs;
    std::map<EnvelopeParams::ParameterType, std::unique_ptr<EnvelopeComponent>> envelopeComponents;

    std::unique_ptr<juce::Label> scaleLabel;
    std::unique_ptr<juce::Slider> scaleSlider;
}; 