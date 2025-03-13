//
// Created by Shaked Melman on 13/03/2025.
//

#ifndef COINCIDENCE_STATEMANAGER_H
#define COINCIDENCE_STATEMANAGER_H

#include <juce_audio_utils/juce_audio_utils.h>
#include "Models.h"
#include "ParameterBinding.h"

namespace AppState {

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    class StateManager {
    public:

        static StateManager &getInstance();

        StateManager() = default;

        void setAudioParametersTree(juce::AudioProcessorValueTreeState *apvts) {
            audioParamsTree = apvts;
        }

        juce::AudioProcessorValueTreeState *getAudioParametersTree() {
            return audioParamsTree;
        }

        template<typename SettingsType>
        std::unique_ptr<ParameterBinding<SettingsType>> createParameterBinding(SettingsType &settings) {
            if (!audioParamsTree)
                return nullptr;
            return std::make_unique<ParameterBinding<SettingsType>>(settings, audioParamsTree);
        }

    private:
        juce::ValueTree tree;
        juce::AudioProcessorValueTreeState *audioParamsTree = nullptr;
    };
}


#endif //COINCIDENCE_STATEMANAGER_H
