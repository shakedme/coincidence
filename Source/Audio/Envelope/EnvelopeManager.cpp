#include "EnvelopeManager.h"
#include "../../Gui/Components/EnvelopeComponent.h"
#include "../../Gui/Sections/EnvelopeSection.h"

EnvelopeManager::EnvelopeManager(EnvelopeParams::Registry& registry)
    : registry(registry)
{
    // Create mappers for all registered envelope parameter types
    for (const auto& typeInfo : registry.getAvailableTypes()) {
        registerEnvelopeParameter(typeInfo.type);
    }
}

EnvelopeManager::~EnvelopeManager() {
    // Clear component map to avoid potential dangling pointers
    envelopeComponentMap.clear();
}

void EnvelopeManager::registerEnvelopeParameter(EnvelopeParams::ParameterType type) {
    // Create a new parameter mapper if it doesn't exist
    if (envelopeParamsMap.find(type) == envelopeParamsMap.end()) {
        envelopeParamsMap[type] = std::make_unique<EnvelopeParameterMapper>(type);
    }
}

EnvelopeParameterMapper* EnvelopeManager::getMapper(EnvelopeParams::ParameterType type) {
    // Ensure the mapper exists
    registerEnvelopeParameter(type);
    return envelopeParamsMap[type].get();
}

void EnvelopeManager::connectComponent(EnvelopeComponent* component, EnvelopeParams::ParameterType type) {
    if (component == nullptr) return;

    // Store the component
    envelopeComponentMap[type] = component;
    
    // Get the parameter mapper
    auto* mapper = getMapper(type);
    
    // Configure the component
    component->setParameterType(type);
    component->setRate(mapper->getRate());
    
    // Set up callback to sync points when they change in the UI
    component->onPointsChanged = [this, type]() {
        // Get points from the component and update our envelope
        if (auto* component = envelopeComponentMap[type]) {
            envelopeParamsMap[type]->setPoints(component->getPoints());
        }
    };
    
    // Set up callback to sync rate changes from the UI
    component->onRateChanged = [this, type](float newRate) {
        // Update envelope rate when UI rate changes
        if (envelopeParamsMap.find(type) != envelopeParamsMap.end()) {
            envelopeParamsMap[type]->setRate(newRate);
        }
    };
}

void EnvelopeManager::applyEnvelopeToBuffer(EnvelopeParams::ParameterType type, juce::AudioBuffer<float>& buffer) {
    // Check if we have this parameter type
    if (envelopeParamsMap.find(type) == envelopeParamsMap.end()) {
        return;
    }
    
    // Get the mapper
    auto* mapper = envelopeParamsMap[type].get();
    
    // Apply envelope to each sample in all channels
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int sample = 0; sample < numSamples; ++sample) {
        // Get current envelope value
        float envelopeValue = mapper->getCurrentValue();

        // Apply to all channels
        for (int channel = 0; channel < numChannels; ++channel) {
            float* channelData = buffer.getWritePointer(channel);
            channelData[sample] *= envelopeValue;
        }
    }
}

void EnvelopeManager::pushAudioBuffer(const float* audioData, int numSamples) {
    // Push audio data to all connected envelope components
    for (auto& pair : envelopeComponentMap) {
        if (pair.second != nullptr) {
            pair.second->pushAudioBuffer(audioData, numSamples);
        }
    }
}

void EnvelopeManager::updateTransportPosition(double ppqPosition) {
    // Update transport position for all envelopes
    for (auto& pair : envelopeParamsMap) {
        pair.second->setTransportPosition(ppqPosition);
    }
}

void EnvelopeManager::connectAllComponents(EnvelopeSectionComponent* section) {
    if (section == nullptr) return;
    
    // Get all available envelope types
    const auto& availableTypes = registry.getAvailableTypes();
    
    // Connect each visible envelope type component
    for (const auto& typeInfo : availableTypes) {
        if (typeInfo.visible) {
            auto component = section->getEnvelopeComponent(typeInfo.type);
            if (component != nullptr) {
                connectComponent(component, typeInfo.type);
            }
        }
    }
}