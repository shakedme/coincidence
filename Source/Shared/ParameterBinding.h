//
// Created by Shaked Melman on 13/03/2025.
//

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <functional>
#include <map>
#include <vector>

namespace AppState {

// Parameter descriptor with member pointer support
    template<typename SettingsType>
    struct ParameterDescriptor {
        juce::String paramID;
        std::function<void(SettingsType &, float)> setter;

        // Standard constructor
        ParameterDescriptor(const juce::String &id,
                            std::function<void(SettingsType &, float)> setterFunc)
                : paramID(id), setter(setterFunc) {}

        // Member pointer constructor with converter
        template<typename ValueType>
        ParameterDescriptor(const juce::String &id,
                            ValueType SettingsType::* memberPtr,
                            std::function<ValueType(float)> converter)
                : paramID(id),
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
        ParameterBinding(SettingsType &settingsStruct, juce::AudioProcessorValueTreeState *apvts)
                : settings(settingsStruct), audioParamsTree(apvts) {
        }

        ~ParameterBinding() {
            removeAllListeners();
        }

        // Register a single parameter descriptor
        void registerParameter(const ParameterDescriptor<SettingsType> &descriptor) {
            if (!audioParamsTree)
                return;

            parameterMap[descriptor.paramID] = descriptor.setter;
            audioParamsTree->addParameterListener(descriptor.paramID, this);

            // Initialize with current value
            if (auto *param = audioParamsTree->getParameter(descriptor.paramID))
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
            if (audioParamsTree) {
                for (const auto &mapping: parameterMap)
                    audioParamsTree->removeParameterListener(mapping.first, this);
                parameterMap.clear();
            }
        }

    private:
        SettingsType &settings;
        juce::AudioProcessorValueTreeState *audioParamsTree;
        std::map<juce::String, std::function<void(SettingsType &, float)>> parameterMap;
    };


// Create a set of delay parameters
    inline std::vector<ParameterDescriptor<Config::FxSettings>> createDelayParameters() {
        return {
                createPercentageParam<Config::FxSettings>("delay_mix", &Config::FxSettings::delayMix),
                createPercentageParam<Config::FxSettings>("delay_feedback", &Config::FxSettings::delayFeedback),
                createPercentageParam<Config::FxSettings>("delay_rate", &Config::FxSettings::delayRate),
                createBoolParam<Config::FxSettings>("delay_ping_pong", &Config::FxSettings::delayPingPong),
                createBoolParam<Config::FxSettings>("delay_bpm_sync", &Config::FxSettings::delayBpmSync)
        };
    }

// Create a set of reverb parameters
    inline std::vector<ParameterDescriptor<Config::FxSettings>> createReverbParameters() {
        return {
                createPercentageParam<Config::FxSettings>("reverb_mix", &Config::FxSettings::reverbMix),
                createPercentageParam<Config::FxSettings>("reverb_time", &Config::FxSettings::reverbTime),
                createPercentageParam<Config::FxSettings>("reverb_width", &Config::FxSettings::reverbWidth)
        };
    }

    // Create melody parameters
    inline std::vector<ParameterDescriptor<Config::MelodySettings>> createMelodyParameters() {
        return {
                createIntParam<Config::MelodySettings>("semitones", &Config::MelodySettings::semitoneValue),
                createIntParam<Config::MelodySettings>("octaves", &Config::MelodySettings::octaveValue),
                createPercentageParam<Config::MelodySettings>("semitones_prob",
                                                              &Config::MelodySettings::semitoneProbability),
                createPercentageParam<Config::MelodySettings>("octaves_prob",
                                                              &Config::MelodySettings::octaveProbability),
                createEnumParam<Config::MelodySettings, Config::DirectionType>("semitones_direction",
                                                                               &Config::MelodySettings::semitoneDirection),
                createEnumParam<Config::MelodySettings, Config::ScaleType>("scaleType",
                                                                           &Config::MelodySettings::scaleType)
        };
    }

    inline std::vector<ParameterDescriptor<Config::MidiSettings>> createMidiParameters() {
        return {
                createPercentageParam<Config::MidiSettings>("gate", &Config::MidiSettings::gateValue),
                createPercentageParam<Config::MidiSettings>("gate_randomize", &Config::MidiSettings::gateRandomize),
                createEnumParam<Config::MidiSettings, Config::DirectionType>("gate_direction",
                                                                             &Config::MidiSettings::gateDirection),
                createPercentageParam<Config::MidiSettings>("velocity", &Config::MidiSettings::velocityValue),
                createPercentageParam<Config::MidiSettings>("velocity_randomize",
                                                            &Config::MidiSettings::velocityRandomize),
                createEnumParam<Config::MidiSettings, Config::DirectionType>("velocity_direction",
                                                                             &Config::MidiSettings::velocityDirection),
                createPercentageParam<Config::MidiSettings>("probability", &Config::MidiSettings::probability),
                createEnumParam<Config::MidiSettings, Config::RhythmMode>("rhythm_mode",
                                                                          &Config::MidiSettings::rhythmMode),
                createPercentageParam<Config::MidiSettings>("1/1", &Config::MidiSettings::barProbability),
                createPercentageParam<Config::MidiSettings>("1/2",
                                                            &Config::MidiSettings::halfBarProbability),
                createPercentageParam<Config::MidiSettings>("1/4",
                                                            &Config::MidiSettings::quarterBarProbability),
                createPercentageParam<Config::MidiSettings>("1/8",
                                                            &Config::MidiSettings::eighthBarProbability),
                createPercentageParam<Config::MidiSettings>("1/16",
                                                            &Config::MidiSettings::sixteenthBarProbability),
                createPercentageParam<Config::MidiSettings>("1/32",
                                                            &Config::MidiSettings::thirtySecondBarProbability)
        };
    }

} // namespace AppState

