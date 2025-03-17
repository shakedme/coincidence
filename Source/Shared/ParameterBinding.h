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
    static const juce::String ID_REVERB_PROBABILITY = "reverb_probability";
    static const juce::String ID_REVERB_TIME = "reverb_time";
    static const juce::String ID_REVERB_WIDTH = "reverb_width";
    // Delay parameters
    static const juce::String ID_DELAY_MIX = "delay_mix";
    static const juce::String ID_DELAY_PROBABILITY = "delay_probability";
    static const juce::String ID_DELAY_RATE = "delay_rate";
    static const juce::String ID_DELAY_FEEDBACK = "delay_feedback";
    static const juce::String ID_DELAY_PING_PONG = "delay_ping_pong";
    static const juce::String ID_DELAY_BPM_SYNC = "delay_bpm_sync";

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

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

    // Create a set of sample parameters
    inline std::vector<ParameterDescriptor<Models::SamplerSettings>> createSampleParameters() {
        return {
                createEnumParam<Models::SamplerSettings, Models::DirectionType>(ID_SAMPLE_DIRECTION,
                                                                                &Models::SamplerSettings::sampleDirection),
                createBoolParam<Models::SamplerSettings>(ID_SAMPLE_PITCH_FOLLOW,
                                                         &Models::SamplerSettings::samplePitchFollow)
        };
    }

    // Create a set of stutter parameters
    inline std::vector<ParameterDescriptor<Models::StutterSettings>> createStutterParameters() {
        return {
                createPercentageParam<Models::StutterSettings>(ID_STUTTER_PROBABILITY,
                                                               &Models::StutterSettings::stutterProbability),
        };
    }

// Create a set of delay parameters
    inline std::vector<ParameterDescriptor<Models::DelaySettings>> createDelayParameters() {
        return {
                createPercentageParam<Models::DelaySettings>(ID_DELAY_MIX, &Models::DelaySettings::delayMix),
                createPercentageParam<Models::DelaySettings>(ID_DELAY_FEEDBACK, &Models::DelaySettings::delayFeedback),
                createPercentageParam<Models::DelaySettings>(ID_DELAY_RATE, &Models::DelaySettings::delayRate),
                createBoolParam<Models::DelaySettings>(ID_DELAY_PING_PONG, &Models::DelaySettings::delayPingPong),
                createBoolParam<Models::DelaySettings>(ID_DELAY_BPM_SYNC, &Models::DelaySettings::delayBpmSync),
                createPercentageParam<Models::DelaySettings>(ID_DELAY_PROBABILITY,
                                                             &Models::DelaySettings::delayProbability)
        };
    }

// Create a set of reverb parameters
    inline std::vector<ParameterDescriptor<Models::ReverbSettings>> createReverbParameters() {
        return {
                createPercentageParam<Models::ReverbSettings>(ID_REVERB_MIX, &Models::ReverbSettings::reverbMix),
                createPercentageParam<Models::ReverbSettings>(ID_REVERB_PROBABILITY,
                                                              &Models::ReverbSettings::reverbProbability),
                createPercentageParam<Models::ReverbSettings>(ID_REVERB_TIME, &Models::ReverbSettings::reverbTime),
                createPercentageParam<Models::ReverbSettings>(ID_REVERB_WIDTH, &Models::ReverbSettings::reverbWidth)
        };
    }

    // Create melody parameters
    inline std::vector<ParameterDescriptor<Models::MelodySettings>> createMelodyParameters() {
        return {
                createIntParam<Models::MelodySettings>(ID_SEMITONES, &Models::MelodySettings::semitoneValue),
                createIntParam<Models::MelodySettings>(ID_OCTAVES, &Models::MelodySettings::octaveValue),
                createPercentageParam<Models::MelodySettings>(ID_SEMITONES_PROB,
                                                              &Models::MelodySettings::semitoneProbability),
                createPercentageParam<Models::MelodySettings>(ID_OCTAVES_PROB,
                                                              &Models::MelodySettings::octaveProbability),
                createEnumParam<Models::MelodySettings, Models::DirectionType>(ID_SEMITONES_DIRECTION,
                                                                               &Models::MelodySettings::semitoneDirection),
                createEnumParam<Models::MelodySettings, Models::ScaleType>(ID_SCALE_TYPE,
                                                                           &Models::MelodySettings::scaleType)
        };
    }

    inline std::vector<ParameterDescriptor<Models::MidiSettings>> createMidiParameters() {
        return {
                createPercentageParam<Models::MidiSettings>(ID_GATE, &Models::MidiSettings::gateValue),
                createPercentageParam<Models::MidiSettings>(ID_GATE_RANDOMIZE, &Models::MidiSettings::gateRandomize),
                createEnumParam<Models::MidiSettings, Models::DirectionType>(ID_GATE_DIRECTION,
                                                                             &Models::MidiSettings::gateDirection),
                createPercentageParam<Models::MidiSettings>(ID_VELOCITY, &Models::MidiSettings::velocityValue),
                createPercentageParam<Models::MidiSettings>(ID_VELOCITY_RANDOMIZE,
                                                            &Models::MidiSettings::velocityRandomize),
                createEnumParam<Models::MidiSettings, Models::DirectionType>(ID_VELOCITY_DIRECTION,
                                                                             &Models::MidiSettings::velocityDirection),
                createPercentageParam<Models::MidiSettings>(ID_PROBABILITY, &Models::MidiSettings::probability),
                createEnumParam<Models::MidiSettings, Models::RhythmMode>(ID_RHYTHM_MODE,
                                                                          &Models::MidiSettings::rhythmMode),
                createPercentageParam<Models::MidiSettings>(ID_RHYTHM_1_1, &Models::MidiSettings::barProbability),
                createPercentageParam<Models::MidiSettings>(ID_RHYTHM_1_2,
                                                            &Models::MidiSettings::halfBarProbability),
                createPercentageParam<Models::MidiSettings>(ID_RHYTHM_1_4,
                                                            &Models::MidiSettings::quarterBarProbability),
                createPercentageParam<Models::MidiSettings>(ID_RHYTHM_1_8,
                                                            &Models::MidiSettings::eighthBarProbability),
                createPercentageParam<Models::MidiSettings>(ID_RHYTHM_1_16,
                                                            &Models::MidiSettings::sixteenthBarProbability),
                createPercentageParam<Models::MidiSettings>(ID_RHYTHM_1_32,
                                                            &Models::MidiSettings::thirtySecondBarProbability)
        };
    }


} // namespace AppState

