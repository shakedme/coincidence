#include "EnvelopeManager.h"
#include "../../Gui/Components/EnvelopeComponent.h"
#include "../../Gui/Sections/EnvelopeSection.h"

EnvelopeManager::EnvelopeManager(EnvelopeParams::Registry &registry)
        : registry(registry) {
    // Create mappers for all registered envelope parameter types
    for (const auto &typeInfo: registry.getAvailableTypes()) {
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

EnvelopeParameterMapper *EnvelopeManager::getMapper(EnvelopeParams::ParameterType type) {
    // Ensure the mapper exists
    registerEnvelopeParameter(type);
    return envelopeParamsMap[type].get();
}

void EnvelopeManager::connectComponent(const std::weak_ptr<EnvelopeComponent> &component,
                                       EnvelopeParams::ParameterType type) {
    // Store the component
    envelopeComponentMap[type] = component;

    auto sharedComponent = component.lock();
    auto *mapper = getMapper(type);

    // Configure the component
    sharedComponent->setParameterType(type);
    sharedComponent->setRate(mapper->getRate());
    sharedComponent->setSettings(registry.getTypeInfo(type).settings);

    // Set up callback to sync points when they change in the UI
    sharedComponent->onPointsChanged = [this, type](const std::vector<std::unique_ptr<EnvelopePoint>> &points) {
        envelopeParamsMap[type]->setPoints(points);
    };

    // Set up callback to sync rate changes from the UI
    sharedComponent->onRateChanged = [this, type](float newRate) {
        // Update envelope rate when UI rate changes
        if (envelopeParamsMap.find(type) != envelopeParamsMap.end()) {
            envelopeParamsMap[type]->setRate(newRate);
        }
    };
}

void EnvelopeManager::pushAudioBuffer(const float *audioData, int numSamples) {
    // Push audio data to all connected envelope components
    for (auto &pair: envelopeComponentMap) {
        if (auto component = pair.second.lock()) {
            component->pushAudioBuffer(audioData, numSamples);
        }
    }
}

void EnvelopeManager::updateTransportPosition(double ppqPosition) {
    // Update transport position for all envelopes
    for (auto &pair: envelopeParamsMap) {
        pair.second->setTransportPosition(ppqPosition);
    }
}

void EnvelopeManager::connectAllComponents(EnvelopeSectionComponent *section) {
    if (section == nullptr) return;

    // Get all available envelope types
    const auto &availableTypes = registry.getAvailableTypes();

    // Connect each visible envelope type component
    for (const auto &typeInfo: availableTypes) {
        if (typeInfo.visible) {
            auto component = section->getEnvelopeComponent(typeInfo.type);
            connectComponent(component, typeInfo.type);
        }
    }
}