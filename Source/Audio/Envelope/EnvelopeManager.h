#pragma once

#include <map>
#include <memory>
#include <string>
#include "EnvelopeParameterMapper.h"
#include "EnvelopeParameterTypes.h"
#include "../../Shared/TimingManager.h"

// Forward declarations
class EnvelopeComponent;

class EnvelopeSectionComponent;

/**
 * Manages multiple envelope instances in the plugin.
 * Provides methods to create, retrieve, delete and apply envelopes.
 */
class EnvelopeManager {
public:
    EnvelopeManager(EnvelopeParams::Registry &registry);

    ~EnvelopeManager();

    // Register a new envelope parameter type
    void registerEnvelopeParameter(EnvelopeParams::ParameterType type);

    // Get a mapper for a specific parameter type
    EnvelopeParameterMapper *getMapper(EnvelopeParams::ParameterType type);

    // Connect a UI component to a parameter type
    void connectComponent(const std::weak_ptr<EnvelopeComponent>& component, EnvelopeParams::ParameterType type);

    // Auto-connect all components from an EnvelopeSection
    void connectAllComponents(EnvelopeSectionComponent *section);

    // Send audio data to all envelope components for visualization
    void pushAudioBuffer(const float *audioData, int numSamples);

    // Update all envelopes with the current transport position
    void updateTransportPosition(double ppqPosition);

    // Access to the envelope registry
    EnvelopeParams::Registry &getRegistry() { return registry; }

private:
    // Storage for all envelope parameter mappers
    std::map<EnvelopeParams::ParameterType, std::unique_ptr<EnvelopeParameterMapper>> envelopeParamsMap;

    // Mapping of UI components to parameter types
    std::map<EnvelopeParams::ParameterType, std::weak_ptr<EnvelopeComponent>> envelopeComponentMap;

    // Reference to envelope registry
    EnvelopeParams::Registry &registry;
};