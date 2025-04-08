//
// Created by Shaked Melman on 30/03/2025.
//


#include "ModulationMatrix.h"
#include "../Gui/Components/Envelope/EnvelopeComponent.h"
#include "../Audio/PluginProcessor.h"
#include <utility>


ModulationMatrix::ModulationMatrix(PluginProcessor &processor) : processor(processor) {}

void ModulationMatrix::addConnection(std::shared_ptr<EnvelopeComponent> &lfo, juce::Identifier paramId) {
    if (isConnected(lfo, paramId)) {
        return;
    }
    auto paramMapper = std::make_unique<EnvelopeParameterMapper>(paramId, processor.getTimingManager());
    connectionsMap[lfo].push_back(std::move(paramMapper));
}

bool ModulationMatrix::isConnected(std::shared_ptr<EnvelopeComponent> &lfo, juce::Identifier paramId) {
    if (connectionsMap.contains(lfo)) {
        auto &sourceConnections = connectionsMap[lfo];
        return std::any_of(sourceConnections.begin(), sourceConnections.end(),
                           [paramId](const std::unique_ptr<EnvelopeParameterMapper> &mapper) {
                               return mapper->getParameterId() == paramId;
                           });
    }
    return false;
}

void ModulationMatrix::removeConnection(std::shared_ptr<EnvelopeComponent> &lfo, juce::Identifier paramId) {
    auto &sourceConnections = connectionsMap[lfo];
    sourceConnections.erase(std::remove_if(sourceConnections.begin(), sourceConnections.end(),
                                           [paramId](const std::unique_ptr<EnvelopeParameterMapper> &mapper) {
                                               return mapper->getParameterId() == paramId;
                                           }), sourceConnections.end());
}

void ModulationMatrix::clearConnections() {
    connectionsMap.clear();
}

void ModulationMatrix::calculateModulationValues() {
    modulationValues.clear();
    for (const auto &[envelopeComponent, paramMappers]: connectionsMap) {
        for (const auto &mapper: paramMappers) {
            mapper->setPoints(envelopeComponent->getPoints());
            mapper->setRate(envelopeComponent->getRate());
            float value = mapper->getCurrentValue();
            modulationValues[mapper->getParameterId()] = value;
        }
    }
}

std::pair<float, float> ModulationMatrix::getParamModulationValue(const juce::Identifier &paramId) {
    float paramValue = processor.getAPVTS().getRawParameterValue(paramId)->load();
    if (modulationValues.contains(paramId)) {
        return std::make_pair(paramValue, modulationValues[paramId]);
    }
    return std::make_pair(paramValue, 1.0f);
}