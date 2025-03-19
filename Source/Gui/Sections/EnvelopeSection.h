#pragma once

#include "../Components/Envelope/EnvelopeTabs.h"
#include "../Components/Envelope/ADSREnvelopeComponent.h"
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
    // ADSR component for amplitude envelope
    std::unique_ptr<ADSREnvelopeComponent> adsrComponent;
    
    // Tabbed interface for LFO envelopes
    std::unique_ptr<EnvelopeTabs> lfoTabs;
    std::unordered_map<int, std::unique_ptr<EnvelopeComponent>> lfoComponents;
    
    // Creates the LFO envelope components
    void createLFOComponents();
}; 