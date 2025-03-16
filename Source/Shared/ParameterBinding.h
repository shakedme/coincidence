//
// Created by Shaked Melman on 13/03/2025.
//

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <functional>
#include <map>
#include <utility>
#include <vector>

namespace AppState {

// Parameter descriptor with member pointer support
    template<typename SettingsType>
    struct ParameterDescriptor {
        juce::String paramID;
        std::function<void(SettingsType &, float)> setter;

        // Standard constructor
        ParameterDescriptor(juce::String id,
                            std::function<void(SettingsType &, float)> setterFunc)
                : paramID(std::move(id)), setter(setterFunc) {}

        // Member pointer constructor with converter
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

// For integer parameters
    template<typename SettingsType>
    ParameterDescriptor<SettingsType> createIntParam(const juce::String &paramID, int SettingsType::* memberPtr) {
        return ParameterDescriptor<SettingsType>(
                paramID,
                memberPtr,
                std::function<int(float)>([](float value) { return static_cast<int>(value); })
        );
    }

// For choice parameters (returning the index directly)
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
        std::map<juce::String, std::function<void(SettingsType &, float)>> parameterMap;
    };

    template<typename SettingsType>
    std::unique_ptr<ParameterBinding<SettingsType>>
    createParameterBinding(SettingsType &settings, juce::AudioProcessorValueTreeState &apvts) {
        return std::make_unique<ParameterBinding<SettingsType>>(settings, apvts);
    }

    // Create a set of stutter parameters
    inline std::vector<ParameterDescriptor<Models::StutterSettings>> createStutterParameters() {
        return {
                createPercentageParam<Models::StutterSettings>("stutter_probability",
                                                               &Models::StutterSettings::stutterProbability),
        };
    }

// Create a set of delay parameters
    inline std::vector<ParameterDescriptor<Models::DelaySettings>> createDelayParameters() {
        return {
                createPercentageParam<Models::DelaySettings>("delay_mix", &Models::DelaySettings::delayMix),
                createPercentageParam<Models::DelaySettings>("delay_feedback", &Models::DelaySettings::delayFeedback),
                createPercentageParam<Models::DelaySettings>("delay_rate", &Models::DelaySettings::delayRate),
                createBoolParam<Models::DelaySettings>("delay_ping_pong", &Models::DelaySettings::delayPingPong),
                createBoolParam<Models::DelaySettings>("delay_bpm_sync", &Models::DelaySettings::delayBpmSync)
        };
    }

// Create a set of reverb parameters
    inline std::vector<ParameterDescriptor<Models::ReverbSettings>> createReverbParameters() {
        return {
                createPercentageParam<Models::ReverbSettings>("reverb_mix", &Models::ReverbSettings::reverbMix),
                createPercentageParam<Models::ReverbSettings>("reverb_time", &Models::ReverbSettings::reverbTime),
                createPercentageParam<Models::ReverbSettings>("reverb_width", &Models::ReverbSettings::reverbWidth)
        };
    }

    // Create melody parameters
    inline std::vector<ParameterDescriptor<Models::MelodySettings>> createMelodyParameters() {
        return {
                createIntParam<Models::MelodySettings>("semitones", &Models::MelodySettings::semitoneValue),
                createIntParam<Models::MelodySettings>("octaves", &Models::MelodySettings::octaveValue),
                createPercentageParam<Models::MelodySettings>("semitones_prob",
                                                              &Models::MelodySettings::semitoneProbability),
                createPercentageParam<Models::MelodySettings>("octaves_prob",
                                                              &Models::MelodySettings::octaveProbability),
                createEnumParam<Models::MelodySettings, Models::DirectionType>("semitones_direction",
                                                                               &Models::MelodySettings::semitoneDirection),
                createEnumParam<Models::MelodySettings, Models::ScaleType>("scaleType",
                                                                           &Models::MelodySettings::scaleType)
        };
    }

    inline std::vector<ParameterDescriptor<Models::MidiSettings>> createMidiParameters() {
        return {
                createPercentageParam<Models::MidiSettings>("gate", &Models::MidiSettings::gateValue),
                createPercentageParam<Models::MidiSettings>("gate_randomize", &Models::MidiSettings::gateRandomize),
                createEnumParam<Models::MidiSettings, Models::DirectionType>("gate_direction",
                                                                             &Models::MidiSettings::gateDirection),
                createPercentageParam<Models::MidiSettings>("velocity", &Models::MidiSettings::velocityValue),
                createPercentageParam<Models::MidiSettings>("velocity_randomize",
                                                            &Models::MidiSettings::velocityRandomize),
                createEnumParam<Models::MidiSettings, Models::DirectionType>("velocity_direction",
                                                                             &Models::MidiSettings::velocityDirection),
                createPercentageParam<Models::MidiSettings>("probability", &Models::MidiSettings::probability),
                createEnumParam<Models::MidiSettings, Models::RhythmMode>("rhythm_mode",
                                                                          &Models::MidiSettings::rhythmMode),
                createPercentageParam<Models::MidiSettings>("1/1", &Models::MidiSettings::barProbability),
                createPercentageParam<Models::MidiSettings>("1/2",
                                                            &Models::MidiSettings::halfBarProbability),
                createPercentageParam<Models::MidiSettings>("1/4",
                                                            &Models::MidiSettings::quarterBarProbability),
                createPercentageParam<Models::MidiSettings>("1/8",
                                                            &Models::MidiSettings::eighthBarProbability),
                createPercentageParam<Models::MidiSettings>("1/16",
                                                            &Models::MidiSettings::sixteenthBarProbability),
                createPercentageParam<Models::MidiSettings>("1/32",
                                                            &Models::MidiSettings::thirtySecondBarProbability)
        };
    }

} // namespace AppState

