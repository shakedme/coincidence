#include "MidiGeneratorParams.h"

namespace MidiGeneratorParams {

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Rate parameters
    const char* rateNames[MidiGeneratorParams::NUM_RATE_OPTIONS] = {"1/2", "1/4", "1/8", "1/16", "1/32"};

    for (int i = 0; i < MidiGeneratorParams::NUM_RATE_OPTIONS; ++i)
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

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "randomize_samples", "Randomize Samples", false));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        "randomize_probability", "Randomize Probability", 0.0f, 100.0f, 100.0f));

    return layout;
}

} // namespace MidiGeneratorParams
