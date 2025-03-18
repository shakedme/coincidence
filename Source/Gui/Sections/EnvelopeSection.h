#pragma once

#include "../Components/Envelope/EnvelopeTabs.h"
#include "BaseSection.h"
#include <map>
#include "../../Shared/Envelope/EnvelopeParameterTypes.h"

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

    std::weak_ptr<EnvelopeComponent> getEnvelopeComponent(EnvelopeParams::ParameterType type);

private:
    std::unique_ptr<EnvelopeTabs> envTabs;
    std::map<EnvelopeParams::ParameterType, std::shared_ptr<EnvelopeComponent>> envelopeComponents;

    std::unique_ptr<juce::Label> scaleLabel;
    std::unique_ptr<juce::Slider> scaleSlider;
}; 