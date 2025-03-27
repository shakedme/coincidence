//
// Created by Shaked Melman on 13/03/2025.
//

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <functional>
#include <map>
#include <utility>
#include <vector>
#include "Models.h"
#include "TimingManager.h"

namespace AppState {


    // Audio params

    // Probability parameters
    static const juce::String ID_PROBABILITY = "probability";
    // Gate parameters
    static const juce::String ID_GATE = "gate";
    static const juce::String ID_GATE_RANDOMIZE = "gate_randomize";
    static const juce::String ID_GATE_DIRECTION = "gate_direction";
    // Velocity parameters
    static const juce::String ID_VELOCITY = "velocity";
    static const juce::String ID_VELOCITY_RANDOMIZE = "velocity_randomize";
    static const juce::String ID_VELOCITY_DIRECTION = "velocity_direction";
    // Rhythm mode parameters
    static const juce::String ID_RHYTHM_MODE = "rhythm_mode";
    // Rhythm subdivision parameters
    static const juce::String ID_RHYTHM_1_1 = "1/1";
    static const juce::String ID_RHYTHM_1_2 = "1/2";
    static const juce::String ID_RHYTHM_1_4 = "1/4";
    static const juce::String ID_RHYTHM_1_8 = "1/8";
    static const juce::String ID_RHYTHM_1_16 = "1/16";
    static const juce::String ID_RHYTHM_1_32 = "1/32";
    // Scale parameters
    static const juce::String ID_SCALE_TYPE = "scale_type";
    // Semitone parameters
    static const juce::String ID_SEMITONES = "semitones";
    static const juce::String ID_SEMITONES_PROB = "semitones_prob";
    static const juce::String ID_SEMITONES_DIRECTION = "semitones_direction";
    // Octave parameters
    static const juce::String ID_OCTAVES = "octaves";
    static const juce::String ID_OCTAVES_PROB = "octaves_prob";
    // Sample parameters
    static const juce::String ID_SAMPLE_DIRECTION = "sample_direction";
    static const juce::String ID_SAMPLE_PITCH_FOLLOW = "sample_pitch_follow";
    // Stutter parameters
    static const juce::String ID_STUTTER_PROBABILITY = "stutter_probability";
    // Reverb parameters
    static const juce::String ID_REVERB_MIX = "reverb_mix";
    static const juce::String ID_REVERB_TIME = "reverb_time";
    static const juce::String ID_REVERB_WIDTH = "reverb_width";

    // Delay parameters
    static const juce::String ID_DELAY_MIX = "delay_mix";
    static const juce::String ID_DELAY_RATE = "delay_rate";
    static const juce::String ID_DELAY_FEEDBACK = "delay_feedback";
    static const juce::String ID_DELAY_PING_PONG = "delay_ping_pong";
    static const juce::String ID_DELAY_BPM_SYNC = "delay_bpm_sync";

    // ADSR parameters
    static const juce::String ID_ADSR_ATTACK = "adsr_attack";
    static const juce::String ID_ADSR_DECAY = "adsr_decay";
    static const juce::String ID_ADSR_SUSTAIN = "adsr_sustain";
    static const juce::String ID_ADSR_RELEASE = "adsr_release";

    // Internal state params

    static const juce::Identifier ID_AMPLITUDE_ENVELOPE = "amplitude_envelope";
    static const juce::Identifier ID_REVERB_ENV = "reverb_envelope";

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    template<typename SettingsType>
    struct ParameterDescriptor {
        juce::String paramID;
        std::function<void(SettingsType &, float)> setter;

        template<typename ValueType>
        ParameterDescriptor(juce::String id,
                            ValueType SettingsType::* memberPtr,
                            std::function<ValueType(float)> converter)
                : paramID(std::move(id)),
                  setter([memberPtr, converter](SettingsType &settings, float value) {
                      settings.*memberPtr = converter(value);
                  }) {}
    };


    template<typename SettingsType>
    ParameterDescriptor<SettingsType> createPercentageParam(
            const juce::String &paramID,
            float SettingsType::* memberPtr) {
        return ParameterDescriptor<SettingsType>(
                paramID,
                memberPtr,
                std::function<float(float)>([](float value) { return value / 100.0f; })
        );
    }

    template<typename SettingsType>
    ParameterDescriptor<SettingsType> createBoolParam(
            const juce::String &paramID,
            bool SettingsType::* memberPtr) {
        return ParameterDescriptor<SettingsType>(
                paramID,
                memberPtr,
                std::function<bool(float)>([](float value) { return value > 0.5f; })
        );
    }

    template<typename SettingsType>
    ParameterDescriptor<SettingsType> createIntParam(const juce::String &paramID, int SettingsType::* memberPtr) {
        return ParameterDescriptor<SettingsType>(
                paramID,
                memberPtr,
                std::function<int(float)>([](float value) { return static_cast<int>(value); })
        );
    }

    template<typename SettingsType>
    ParameterDescriptor<SettingsType> createChoiceParam(
            const juce::String &paramID,
            int SettingsType::* memberPtr) {
        return ParameterDescriptor<SettingsType>(
                paramID,
                memberPtr,
                std::function<int(float)>([](float value) { return static_cast<int>(value); })
        );
    }

    template<typename SettingsType, typename EnumType>
    ParameterDescriptor<SettingsType> createEnumParam(
            const juce::String &paramID,
            EnumType SettingsType::* memberPtr) {
        return ParameterDescriptor<SettingsType>(
                paramID,
                memberPtr,
                std::function<EnumType(float)>(
                        [](float value) { return static_cast<EnumType>(static_cast<int>(value)); })
        );
    }

    template<typename SettingsType, typename ValueType>
    ParameterDescriptor<SettingsType> createGenericParam(
            const juce::String &paramID,
            ValueType SettingsType::* memberPtr,
            std::function<ValueType(float)> converter) {
        return ParameterDescriptor<SettingsType>(
                paramID,
                memberPtr,
                converter
        );
    }


    template<typename SettingsType>
    class ParameterBinding : public juce::AudioProcessorValueTreeState::Listener {
    public:
        ParameterBinding(SettingsType &settingsStruct, juce::AudioProcessorValueTreeState &apvts)
                : settings(settingsStruct), audioParamsTree(apvts) {
        }

        ~ParameterBinding() {
            removeAllListeners();
        }

        // Register a single parameter descriptor
        void registerParameter(const ParameterDescriptor<SettingsType> &descriptor) {
            parameterMap[descriptor.paramID] = descriptor.setter;
            audioParamsTree.addParameterListener(descriptor.paramID, this);

            // Initialize with current value
            if (auto *param = audioParamsTree.getParameter(descriptor.paramID))
                descriptor.setter(settings, param->getValue());
        }

        // Register a list of parameter descriptors
        void registerParameters(const std::vector<ParameterDescriptor<SettingsType>> &descriptors) {
            for (const auto &descriptor: descriptors)
                registerParameter(descriptor);
        }

        // Implement the listener callback
        void parameterChanged(const juce::String &parameterID, float newValue) override {
            auto it = parameterMap.find(parameterID);
            if (it != parameterMap.end())
                it->second(settings, newValue);
        }

        void removeAllListeners() {
            for (const auto &mapping: parameterMap)
                audioParamsTree.removeParameterListener(mapping.first, this);
            parameterMap.clear();
        }

    private:
        SettingsType &settings;
        juce::AudioProcessorValueTreeState &audioParamsTree;
        std::unordered_map<juce::String, std::function<void(SettingsType &, float)>> parameterMap;
    };

    template<typename SettingsType>
    std::unique_ptr<ParameterBinding<SettingsType>>
    createParameterBinding(SettingsType &settings, juce::AudioProcessorValueTreeState &apvts) {
        return std::make_unique<ParameterBinding<SettingsType>>(settings, apvts);
    }

    std::vector<ParameterDescriptor<Models::SamplerSettings>> createSampleParameters();

    std::vector<ParameterDescriptor<Models::StutterSettings>> createStutterParameters();

    std::vector<ParameterDescriptor<Models::DelaySettings>> createDelayParameters();

    std::vector<ParameterDescriptor<Models::ReverbSettings>> createReverbParameters();

    std::vector<ParameterDescriptor<Models::MelodySettings>> createMelodyParameters();

    std::vector<ParameterDescriptor<Models::MidiSettings>> createMidiParameters();

    std::vector<ParameterDescriptor<Models::ADSRSettings>> createADSRParameters();

    template<typename ValueType>
    class SingleParameterBinding : public juce::ValueTree::Listener {
    public:
        SingleParameterBinding(ValueType &memberVariable, juce::ValueTree &tree, juce::Identifier paramID)
                : memberVar(memberVariable), valueTree(tree), parameterID(std::move(paramID)) {

            if (!valueTree.hasProperty(parameterID)) {
                valueTree.setProperty(parameterID, memberVar, nullptr);
            };

            valueTree.addListener(this);
            // Initialize with current value
            memberVar = valueTree.getProperty(parameterID);
        }

        ~SingleParameterBinding() {
            valueTree.removeListener(this);
        }

        void valueTreePropertyChanged(juce::ValueTree &tree, const juce::Identifier &property) override {
            if (property == parameterID)
                memberVar = tree.getProperty(property);
        }

    private:
        ValueType &memberVar;
        juce::ValueTree &valueTree;
        juce::Identifier parameterID;
    };

    // Single parameter binding class for binding directly to class member variables
    template<typename ValueType>
    class SingleAudioParameterBinding : public juce::AudioProcessorValueTreeState::Listener {
    public:
        SingleAudioParameterBinding(ValueType &memberVariable,
                                    juce::AudioProcessorValueTreeState &apvts,
                                    juce::String paramID,
                                    std::function<ValueType(float)> converter)
                : memberVar(memberVariable),
                  audioParamsTree(apvts),
                  parameterID(std::move(paramID)),
                  valueConverter(std::move(converter)) {
            audioParamsTree.addParameterListener(parameterID, this);

            // Initialize with current value
            if (auto *param = audioParamsTree.getParameter(parameterID))
                memberVar = valueConverter(param->getValue());
        }

        ~SingleAudioParameterBinding() {
            audioParamsTree.removeParameterListener(parameterID, this);
        }

        void parameterChanged(const juce::String &paramID, float newValue) override {
            if (paramID == parameterID)
                memberVar = valueConverter(newValue);
        }

    private:
        ValueType &memberVar;
        juce::AudioProcessorValueTreeState &audioParamsTree;
        juce::String parameterID;
        std::function<ValueType(float)> valueConverter;
    };

    // Helper function to create a single parameter binding
    template<typename ValueType>
    std::unique_ptr<SingleAudioParameterBinding<ValueType>>
    createSingleParameterBinding(ValueType &memberVariable,
                                 juce::AudioProcessorValueTreeState &apvts,
                                 const juce::String &paramID,
                                 std::function<ValueType(float)> converter) {
        return std::make_unique<SingleAudioParameterBinding<ValueType>>(
                memberVariable, apvts, paramID, converter);
    }

    // Convenience functions for common parameter types
    inline std::unique_ptr<SingleAudioParameterBinding<float>>
    createPercentageParameterBinding(float &memberVariable,
                                     juce::AudioProcessorValueTreeState &apvts,
                                     const juce::String &paramID) {
        return createSingleParameterBinding<float>(
                memberVariable, apvts, paramID,
                [](float value) { return value / 100.0f; });
    }

    inline std::unique_ptr<SingleAudioParameterBinding<bool>>
    createBoolParameterBinding(bool &memberVariable,
                               juce::AudioProcessorValueTreeState &apvts,
                               const juce::String &paramID) {
        return createSingleParameterBinding<bool>(
                memberVariable, apvts, paramID,
                [](float value) { return value > 0.5f; });
    }

    inline std::unique_ptr<SingleAudioParameterBinding<int>>
    createIntParameterBinding(int &memberVariable,
                              juce::AudioProcessorValueTreeState &apvts,
                              const juce::String &paramID) {
        return createSingleParameterBinding<int>(
                memberVariable, apvts, paramID,
                [](float value) { return static_cast<int>(value); });
    }

    template<typename EnumType>
    std::unique_ptr<SingleAudioParameterBinding<EnumType>>
    createEnumParameterBinding(EnumType &memberVariable,
                               juce::AudioProcessorValueTreeState &apvts,
                               const juce::String &paramID) {
        return createSingleParameterBinding<EnumType>(
                memberVariable, apvts, paramID,
                [](float value) { return static_cast<EnumType>(static_cast<int>(value)); });
    }

} // namespace AppState

