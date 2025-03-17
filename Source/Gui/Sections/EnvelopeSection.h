#pragma once

#include "../Components/EnvelopeTabs.h"
#include "BaseSection.h"
#include <map>
#include "../../Audio/Envelope/EnvelopeParameterTypes.h"

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
    EnvelopeComponent* getEnvelopeComponent(EnvelopeParams::ParameterType type);

    // Legacy accessor methods for backward compatibility
    EnvelopeComponent* getAmplitudeEnvelopeComponent() { return getEnvelopeComponent(EnvelopeParams::ParameterType::Amplitude); }
    EnvelopeComponent* getReverbEnvelopeComponent() { return getEnvelopeComponent(EnvelopeParams::ParameterType::Reverb); }

private:
    // Set up UI controls and tabs
    void setupControls();
    void setupScaleSlider();

    // Tab component for envelope selection
    std::unique_ptr<EnvelopeTabs> envTabs;

    // Map of envelope components by type
    std::map<EnvelopeParams::ParameterType, std::unique_ptr<EnvelopeComponent>> envelopeComponents;

    // UI controls
    std::unique_ptr<juce::Label> scaleLabel;
    std::unique_ptr<juce::Slider> scaleSlider;
}; 