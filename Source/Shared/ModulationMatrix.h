//
// Created by Shaked Melman on 30/03/2025.
//

#ifndef COINCIDENCE_MODULATIONMATRIX_H
#define COINCIDENCE_MODULATIONMATRIX_H

#include <juce_audio_utils/juce_audio_utils.h>

class PluginProcessor;

class EnvelopeParameterMapper;

class EnvelopeComponent;


class ModulationMatrix {

public:
    ModulationMatrix(PluginProcessor &processor);

    void addConnection(std::shared_ptr<EnvelopeComponent> &lfo, juce::Identifier paramId);

    void removeConnection(std::shared_ptr<EnvelopeComponent> &lfo, juce::Identifier paramId);

    void clearConnections();

    bool isConnected(std::shared_ptr<EnvelopeComponent> &lfo, juce::Identifier paramId);

    void calculateModulationValues();

    float getParamModulationValue(const juce::Identifier &paramId);

private:
    PluginProcessor &processor;

    std::map<std::shared_ptr<EnvelopeComponent>, std::vector<std::unique_ptr<EnvelopeParameterMapper>>> connectionsMap;
    std::map<juce::Identifier, float> modulationValues;
};


#endif //COINCIDENCE_MODULATIONMATRIX_H
