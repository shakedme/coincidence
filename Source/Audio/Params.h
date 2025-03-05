#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

// Forward declaration
class PluginProcessor;

/**
 * Common types and constants for the MIDI Generator plugin
 */
namespace Params
{

// Rate options
enum RateOption {
    RATE_1_2 = 0,
    RATE_1_4,
    RATE_1_8,
    RATE_1_16,
    RATE_1_32,
    RATE_1_64,
    NUM_RATE_OPTIONS
};

struct FxSettings
{
    float stutterProbability = 0.0f;      // 0-100%
    float reverbMix = 50.0f;              // 0-100% (dry/wet mix)
    float reverbProbability = 0.0f;       // 0-100% (chance of applying reverb)
    float reverbTime = 50.0f;             // 0-100% (reverb decay time)
};

// Scale types
enum ScaleType {
    SCALE_MAJOR = 0,
    SCALE_MINOR,
    SCALE_PENTATONIC,
    NUM_SCALE_TYPES
};

// Rhythm modes
enum RhythmMode {
    RHYTHM_NORMAL = 0,
    RHYTHM_DOTTED,
    RHYTHM_TRIPLET,
    NUM_RHYTHM_MODES
};

// Rate settings
struct RateSettings {
    float value = 0.0f;  // 0-100% intensity
};

enum DirectionType
{
    LEFT = 0,
    BIDIRECTIONAL,
    RIGHT,
    RANDOM
};

// Gate settings
struct GateSettings {
    float value = 50.0f;       // 0-100%
    float randomize = 0.0f;    // 0-100% (how much to randomize the gate)
    DirectionType direction = RIGHT; // Direction of randomization
};

// Velocity settings
struct VelocitySettings {
    float value = 100.0f;      // 0-100%
    float randomize = 0.0f;    // 0-100% (how much to randomize the velocity)
    DirectionType direction = RIGHT; // Direction of randomization
};

// Semitone settings
struct SemitoneSettings {
    int value = 0;             // Number of semitones
    float probability = 0.0f;  // 0-100% chance of modifying note by semitones
    bool bidirectional = false; // Whether to allow negative semitones
    DirectionType direction = BIDIRECTIONAL; // Direction for arpeggiator

};

// Octave settings
struct OctaveSettings {
    int value = 0;            // Number of octaves
    float probability = 0.0f; // 0-100% chance of modifying note by octaves
    bool bidirectional = false; // Whether to allow negative octaves
};

// Generator settings
struct GeneratorSettings {
    // Rhythm settings
    RateSettings rates[Params::NUM_RATE_OPTIONS];
    GateSettings gate;
    RhythmMode rhythmMode = RHYTHM_NORMAL;
    VelocitySettings velocity;
    float probability = 100.0f; // 0-100% chance of triggering a note

    // Melody settings
    ScaleType scaleType = SCALE_MAJOR;
    SemitoneSettings semitones;
    OctaveSettings octaves;
};

// Create the parameter layout for the processor
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

// Scale patterns (semitone intervals from root)
static inline const juce::Array<int> majorScale = {0, 2, 4, 5, 7, 9, 11};
static inline const juce::Array<int> minorScale = {0, 2, 3, 5, 7, 8, 10};
static inline const juce::Array<int> pentatonicScale = {0, 2, 4, 7, 9};

} // namespace MidiGeneratorParams
