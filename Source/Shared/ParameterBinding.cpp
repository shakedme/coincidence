//
// Created by Shaked Melman on 13/03/2025.
//

#include "ParameterBinding.h"

namespace AppState {


    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        // Rate parameters
        for (auto &rateName: Models::rateBaseNames) {
            layout.add(std::make_unique<juce::AudioParameterInt>(
                    rateName,
                    "Rate " + juce::String(rateName) + " Value",
                    0.0f,
                    100.0f,
                    0.0f));
        }

        // Density parameter (overall probability)
        layout.add(std::make_unique<juce::AudioParameterInt>(
                ID_PROBABILITY, "Probability", 0.0f, 100.0f, 100.0f));

        // Gate parameters
        layout.add(
                std::make_unique<juce::AudioParameterInt>(
                        ID_GATE, "Gate", 0.0f, 100.0f, 100.0f));

        layout.add(std::make_unique<juce::AudioParameterInt>(
                ID_GATE_RANDOMIZE, "Gate Randomize", 0.0f, 100.0f, 0.0f));

        // Velocity parameters
        layout.add(std::make_unique<juce::AudioParameterInt>(
                ID_VELOCITY, "Velocity", 0.0f, 100.0f, 100.0f));

        layout.add(std::make_unique<juce::AudioParameterInt>(
                ID_VELOCITY_RANDOMIZE, "Velocity Randomize", 0.0f, 100.0f, 0.0f));

        layout.add(std::make_unique<juce::AudioParameterChoice>(
                ID_RHYTHM_MODE,
                "Rhythm Mode",
                juce::StringArray("Normal", "Dotted", "Triplet"),
                Models::RHYTHM_NORMAL));

        // Scale parameters
        layout.add(std::make_unique<juce::AudioParameterChoice>(
                ID_SCALE_TYPE,
                "Scale Type",
                juce::StringArray("Major", "Minor", "Pentatonic"),
                0));

        // Semitone parameters
        layout.add(
                std::make_unique<juce::AudioParameterInt>(
                        ID_SEMITONES, "Semitones", 0, 12, 0));

        layout.add(std::make_unique<juce::AudioParameterInt>(
                ID_SEMITONES_PROB, "Semitones Probability", 0.0f, 100.0f, 0.0f));

        // Octave parameters
        layout.add(std::make_unique<juce::AudioParameterInt>(
                ID_OCTAVES, "Octaves", 0, 3, 0));

        layout.add(std::make_unique<juce::AudioParameterInt>(
                ID_OCTAVES_PROB, "Octaves Probability", 0.0f, 100.0f, 0.0f));

        // Sample direction parameter
        layout.add(std::make_unique<juce::AudioParameterChoice>(
                ID_SAMPLE_DIRECTION,
                "Sample Direction",
                juce::StringArray("Left", "Bidirectional", "Right", "Random"),
                Models::BIDIRECTIONAL)); // Default to bidirectional

        layout.add(std::make_unique<juce::AudioParameterChoice>(
                ID_GATE_DIRECTION,
                "Gate Direction",
                juce::StringArray("Left", "Bidirectional", "Right", "Random"),
                Models::BIDIRECTIONAL)); // Default to bidirectional

        layout.add(std::make_unique<juce::AudioParameterChoice>(
                ID_VELOCITY_DIRECTION,
                "Velocity Direction",
                juce::StringArray("Left", "Bidirectional", "Right", "Random"),
                Models::BIDIRECTIONAL)); // Default to bidirectional

        layout.add(std::make_unique<juce::AudioParameterChoice>(
                ID_SEMITONES_DIRECTION,
                "Semitones Direction",
                juce::StringArray("Left", "Bidirectional", "Right", "Random"),
                Models::BIDIRECTIONAL)); // Default to bidirectional

        layout.add(std::make_unique<juce::AudioParameterFloat>(
                ID_STUTTER_PROBABILITY, "Stutter Amount", 0.0f, 100.0f, 0.0f));

        layout.add(std::make_unique<juce::AudioParameterBool>(
                ID_SAMPLE_PITCH_FOLLOW, "Sample Pitch Follow", false)); // Default: original pitch

        // Reverb parameters
        layout.add(std::make_unique<juce::AudioParameterFloat>(
                ID_REVERB_MIX, "Reverb Mix", 0.0f, 100.0f, 0.0f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
                ID_REVERB_TIME, "Reverb Time", 0.0f, 100.0f, 20.0f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
                ID_REVERB_WIDTH, "Reverb Width", 0.0f, 100.0f, 50.0f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
                ID_REVERB_ENV, "Reverb Envelope", 0.0f, 100.0f, 0.0f));

        // Delay parameters
        layout.add(std::make_unique<juce::AudioParameterFloat>(
                ID_DELAY_MIX, "Delay Mix", 0.0f, 100.0f, 0.0f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
                ID_DELAY_RATE, "Delay Rate", 0.0f, 100.0f, 50.0f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
                ID_DELAY_FEEDBACK, "Delay Feedback", 0.0f, 100.0f, 50.0f));

        layout.add(std::make_unique<juce::AudioParameterBool>(
                ID_DELAY_PING_PONG, "Delay Ping Pong", false));

        layout.add(std::make_unique<juce::AudioParameterBool>(
                ID_DELAY_BPM_SYNC, "Delay BPM Sync", true));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
                ID_AMPLITUDE_ENVELOPE, "Amplitude Envelope", 0.0f, 100.0f, 0.0f));

        return layout;
    }


    std::vector<ParameterDescriptor<Models::SamplerSettings>> createSampleParameters() {
        return {
                createEnumParam<Models::SamplerSettings, Models::DirectionType>(ID_SAMPLE_DIRECTION,
                                                                                &Models::SamplerSettings::sampleDirection),
                createBoolParam<Models::SamplerSettings>(ID_SAMPLE_PITCH_FOLLOW,
                                                         &Models::SamplerSettings::samplePitchFollow)
        };
    }

    std::vector<ParameterDescriptor<Models::StutterSettings>> createStutterParameters() {
        return {
                createPercentageParam<Models::StutterSettings>(ID_STUTTER_PROBABILITY,
                                                               &Models::StutterSettings::stutterProbability),
        };
    }

    std::vector<ParameterDescriptor<Models::DelaySettings>> createDelayParameters() {
        return {
                createPercentageParam<Models::DelaySettings>(ID_DELAY_MIX, &Models::DelaySettings::delayMix),
                createPercentageParam<Models::DelaySettings>(ID_DELAY_FEEDBACK, &Models::DelaySettings::delayFeedback),
                createPercentageParam<Models::DelaySettings>(ID_DELAY_RATE, &Models::DelaySettings::delayRate),
                createBoolParam<Models::DelaySettings>(ID_DELAY_PING_PONG, &Models::DelaySettings::delayPingPong),
                createBoolParam<Models::DelaySettings>(ID_DELAY_BPM_SYNC, &Models::DelaySettings::delayBpmSync)
        };
    }

    std::vector<ParameterDescriptor<Models::ReverbSettings>> createReverbParameters() {
        return {
                createPercentageParam<Models::ReverbSettings>(ID_REVERB_MIX, &Models::ReverbSettings::reverbMix),
                createPercentageParam<Models::ReverbSettings>(ID_REVERB_TIME, &Models::ReverbSettings::reverbTime),
                createPercentageParam<Models::ReverbSettings>(ID_REVERB_WIDTH, &Models::ReverbSettings::reverbWidth),
                createPercentageParam<Models::ReverbSettings>(ID_REVERB_ENV, &Models::ReverbSettings::reverbEnvelope)
        };
    }

    std::vector<ParameterDescriptor<Models::MelodySettings>> createMelodyParameters() {
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

    std::vector<ParameterDescriptor<Models::MidiSettings>> createMidiParameters() {
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

}
