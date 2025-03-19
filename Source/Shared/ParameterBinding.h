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
    static const juce::String ID_REVERB_ENV = "reverb_envelope";

    // Delay parameters
    static const juce::String ID_DELAY_MIX = "delay_mix";
    static const juce::String ID_DELAY_RATE = "delay_rate";
    static const juce::String ID_DELAY_FEEDBACK = "delay_feedback";
    static const juce::String ID_DELAY_PING_PONG = "delay_ping_pong";
    static const juce::String ID_DELAY_BPM_SYNC = "delay_bpm_sync";

    static const juce::String ID_AMPLITUDE_ENVELOPE = "amplitude_envelope";

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

// Helper functions to create parameter descriptors with common conversions

// For percentage parameters (0-100 to 0.0-1.0)
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

// For boolean parameters
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

// For enum parameters (converting float to enum type)
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

// Parameter binding class to connect APVTS parameters to settings struct members
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

    // Create a set of sample parameters
    std::vector<ParameterDescriptor<Models::SamplerSettings>> createSampleParameters();

    // Create a set of stutter parameters
    std::vector<ParameterDescriptor<Models::StutterSettings>> createStutterParameters();

    // Create a set of delay parameters
    std::vector<ParameterDescriptor<Models::DelaySettings>> createDelayParameters();

    // Create a set of reverb parameters
    std::vector<ParameterDescriptor<Models::ReverbSettings>> createReverbParameters();

    // Create melody parameters
    std::vector<ParameterDescriptor<Models::MelodySettings>> createMelodyParameters();

    // Create MIDI parameters
    std::vector<ParameterDescriptor<Models::MidiSettings>> createMidiParameters();

    // Single parameter binding class for binding directly to class member variables
    template<typename ValueType>
    class SingleParameterBinding : public juce::AudioProcessorValueTreeState::Listener {
    public:
        SingleParameterBinding(ValueType &memberVariable,
                               juce::AudioProcessorValueTreeState &apvts,
                               juce::String paramID,
                               std::function<ValueType(float)> converter)
                : memberVar(memberVariable),
                  audioParamsTree(apvts),
                  parameterID(std::move(paramID)),
                  valueConverter(converter) {
            audioParamsTree.addParameterListener(parameterID, this);

            // Initialize with current value
            if (auto *param = audioParamsTree.getParameter(parameterID))
                memberVar = valueConverter(param->getValue());
        }

        ~SingleParameterBinding() {
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
    std::unique_ptr<SingleParameterBinding<ValueType>>
    createSingleParameterBinding(ValueType &memberVariable,
                                 juce::AudioProcessorValueTreeState &apvts,
                                 const juce::String &paramID,
                                 std::function<ValueType(float)> converter) {
        return std::make_unique<SingleParameterBinding<ValueType>>(
                memberVariable, apvts, paramID, converter);
    }

    // Convenience functions for common parameter types
    inline std::unique_ptr<SingleParameterBinding<float>>
    createPercentageParameterBinding(float& memberVariable,
                                     juce::AudioProcessorValueTreeState& apvts,
                                     const juce::String& paramID) {
        return createSingleParameterBinding<float>(
                memberVariable, apvts, paramID,
                [](float value) { return value / 100.0f; });
    }

    inline std::unique_ptr<SingleParameterBinding<bool>>
    createBoolParameterBinding(bool& memberVariable,
                               juce::AudioProcessorValueTreeState& apvts,
                               const juce::String& paramID) {
        return createSingleParameterBinding<bool>(
                memberVariable, apvts, paramID,
                [](float value) { return value > 0.5f; });
    }

    inline std::unique_ptr<SingleParameterBinding<int>>
    createIntParameterBinding(int& memberVariable,
                              juce::AudioProcessorValueTreeState& apvts,
                              const juce::String& paramID) {
        return createSingleParameterBinding<int>(
                memberVariable, apvts, paramID,
                [](float value) { return static_cast<int>(value); });
    }

    template<typename EnumType>
    std::unique_ptr<SingleParameterBinding<EnumType>>
    createEnumParameterBinding(EnumType& memberVariable,
                               juce::AudioProcessorValueTreeState& apvts,
                               const juce::String& paramID) {
        return createSingleParameterBinding<EnumType>(
                memberVariable, apvts, paramID,
                [](float value) { return static_cast<EnumType>(static_cast<int>(value)); });
    }

} // namespace AppState

