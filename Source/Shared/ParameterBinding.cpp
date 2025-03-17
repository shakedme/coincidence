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

        return layout;
    }

}
