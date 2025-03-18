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

    // Create envelope components for all registered types
    void createEnvelopeComponents();

    // Access to components by type
    std::weak_ptr<EnvelopeComponent> getEnvelopeComponent(EnvelopeParams::ParameterType type);

private:
    // Set up UI controls and tabs
    void setupControls();

    void setupScaleSlider();

    // Tab component for envelope selection
    std::unique_ptr<EnvelopeTabs> envTabs;

    // Map of envelope components by type
    std::map<EnvelopeParams::ParameterType, std::shared_ptr<EnvelopeComponent>> envelopeComponents;

    // UI controls
    std::unique_ptr<juce::Label> scaleLabel;
    std::unique_ptr<juce::Slider> scaleSlider;
}; 