#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <memory>
#include <vector>


class ParameterLoader {
public:
    static juce::var loadParametersJson();

    static void addParameterFromJson(juce::AudioProcessorValueTreeState::ParameterLayout &layout,
                                     const juce::var &parameterData);

    static std::unique_ptr<juce::RangedAudioParameter> createParameterFromJson(const juce::var &paramData);

    static void createDynamicParameters(juce::AudioProcessorValueTreeState::ParameterLayout &layout,
                                        const juce::var &paramData);

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

};