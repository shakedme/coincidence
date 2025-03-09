#include "Params.h"

namespace Params
{

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Rate parameters
    const char* rateNames[Params::NUM_RATE_OPTIONS] = {"1/2", "1/4", "1/8", "1/16", "1/32"};

    for (int i = 0; i < Params::NUM_RATE_OPTIONS; ++i)
    {
        // Rate value parameter (0-100%)
        layout.add(std::make_unique<juce::AudioParameterInt>(
            "rate_" + juce::String(i) + "_value",
            "Rate " + juce::String(rateNames[i]) + " Value",
            0.0f,
            100.0f,
            0.0f)); // Default: 0%
    }

    // Density parameter (overall probability)
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "density", "Density", 0.0f, 100.0f, 50.0f));

    // Gate parameters
    layout.add(
        std::make_unique<juce::AudioParameterInt>("gate", "Gate", 0.0f, 100.0f, 50.0f));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        "gate_randomize", "Gate Randomize", 0.0f, 100.0f, 0.0f));

    // Velocity parameters
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "velocity", "Velocity", 0.0f, 100.0f, 100.0f));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        "velocity_randomize", "Velocity Randomize", 0.0f, 100.0f, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "rhythm_mode",
        "Rhythm Mode",
        juce::StringArray("Normal", "Dotted", "Triplet"),
        RHYTHM_NORMAL));

    // Scale parameters
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "scale_type",
        "Scale Type",
        juce::StringArray("Major", "Minor", "Pentatonic"),
        0));

    // Semitone parameters
    layout.add(
        std::make_unique<juce::AudioParameterInt>("semitones", "Semitones", 0, 12, 0));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        "semitones_prob", "Semitones Probability", 0.0f, 100.0f, 0.0f));

    // Octave parameters
    layout.add(std::make_unique<juce::AudioParameterInt>("octaves", "Octaves", 0, 3, 0));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        "octaves_prob", "Octaves Probability", 0.0f, 100.0f, 0.0f));

    // Sample direction parameter
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "sample_direction",
        "Sample Direction",
        juce::StringArray("Left", "Bidirectional", "Right", "Random"),
        BIDIRECTIONAL)); // Default to bidirectional

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "gate_direction",
        "Gate Direction",
        juce::StringArray("Left", "Bidirectional", "Right", "Random"),
        BIDIRECTIONAL)); // Default to bidirectional

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "velocity_direction",
        "Velocity Direction",
        juce::StringArray("Left", "Bidirectional", "Right", "Random"),
        BIDIRECTIONAL)); // Default to bidirectional

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "semitones_direction",
        "Semitones Direction",
        juce::StringArray("Left", "Bidirectional", "Right", "Random"),
        BIDIRECTIONAL)); // Default to bidirectional

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "glitch_stutter", "Stutter Amount", 0.0f, 100.0f, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "sample_pitch_follow", "Sample Pitch Follow", false)); // Default: original pitch

    // Reverb parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "reverb_mix", "Reverb Mix", 0.0f, 100.0f, 50.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "reverb_probability", "Reverb Probability", 0.0f, 100.0f, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "reverb_time", "Reverb Time", 0.0f, 100.0f, 50.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "reverb_width", "Reverb Width", 0.0f, 100.0f, 100.0f));
        
    // Delay parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "delay_mix", "Delay Mix", 0.0f, 100.0f, 50.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "delay_probability", "Delay Probability", 0.0f, 100.0f, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "delay_rate", "Delay Rate", 0.0f, 100.0f, 50.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "delay_feedback", "Delay Feedback", 0.0f, 100.0f, 50.0f));
        
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "delay_ping_pong", "Delay Ping Pong", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "delay_bpm_sync", "Delay BPM Sync", true));

    return layout;
}

} // namespace MidiGeneratorParams
