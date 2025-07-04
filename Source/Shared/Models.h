#pragma once

#include "juce_audio_utils/juce_audio_utils.h"

class PluginProcessor;
class EnvelopePoint;

/**
 * Common types and constants
 */
namespace Models {

    enum RateOption {
        RATE_1_1 = 0,
        RATE_1_2,
        RATE_1_4,
        RATE_1_8,
        RATE_1_16,
        RATE_1_32,
        NUM_RATE_OPTIONS
    };

    static inline const char *rateBaseNames[Models::NUM_RATE_OPTIONS] = {
            "1/1", "1/2", "1/4", "1/8", "1/16", "1/32"};

    enum class LFORate {
        TwoWhole = 0,
        Whole,
        Half,
        Quarter,
        Eighth,
        Sixteenth,
        ThirtySecond
    };

    enum ScaleType {
        SCALE_MAJOR = 0,
        SCALE_MINOR,
        SCALE_PENTATONIC,
        NUM_SCALE_TYPES
    };

    enum RhythmMode {
        RHYTHM_NORMAL = 0,
        RHYTHM_DOTTED,
        RHYTHM_TRIPLET,
        NUM_RHYTHM_MODES
    };

    enum DirectionType {
        LEFT = 0,
        BIDIRECTIONAL,
        RIGHT,
        RANDOM
    };

    enum EffectType {
        REVERB = 0,
        STUTTER,
        DELAY,
        NUM_EFFECT_TYPES
    };

    struct StutterSettings {
        float stutterProbability = 0.0f;      // 0-100%
    };

    struct ReverbSettings {
        float reverbMix = 50.0f;              // 0-100% (dry/wet mix)
        float reverbTime = 50.0f;             // 0-100% (reverb decay time)
        float reverbWidth = 100.0f;           // 0-100% (stereo width)
    };

    struct DelaySettings {
        float delayMix = 50.0f;               // 0-100% (dry/wet mix)
        float delayRate = 50.0f;              // 0-100% (delay time in milliseconds or BPM sync)
        float delayFeedback = 50.0f;          // 0-100% (amount of feedback in the delay)
        bool delayPingPong = false;           // Ping pong mode (true) or normal (false)
        bool delayBpmSync = true;             // BPM sync mode (true) or milliseconds (false)
    };

    struct CompressionSettings {
        float mix = 0.0f;               // 0-100% (dry/wet mix)
        float threshold = 0.0f;           // -60dB to 0dB
        float ratio = 1.0f;               // 1:1 to 20:1
        float attack = 10.0f;             // 0.1ms to 100ms
        float release = 100.0f;           // 10ms to 1000ms
    };

    struct PanSettings {
        float panPosition = 0.0f; // Normalized 0-1, where 0=Left, 0.5=Center, 1=Right
    };

    struct FlangerSettings {
        float mix = 0.0f;              // Normalized 0-1, dry/wet mix
        float rate = 0.0f;             // Normalized 0-1, maps to 0.01-20 Hz
        float depth = 0.0f;            // Normalized 0-1, modulation depth
        float feedback = 0.0f;         // Normalized 0-1, feedback amount
    };

    struct PhaserSettings {
        float mix = 0.0f;              // Normalized 0-1, dry/wet mix
        float rate = 0.0f;             // Normalized 0-1, maps to 0.01-20 Hz
        float depth = 0.0f;            // Normalized 0-1, modulation depth
        float feedback = 0.0f;         // Normalized 0-1, feedback amount
        int stages = 4;                // Number of filter stages (typically 4, 8, or 12)
    };

    // Generator settings
    struct MidiSettings {
        float probability = 100.0f; // 0-100% chance of triggering a note

        RhythmMode rhythmMode = RHYTHM_NORMAL;

        // Rhythm settings
        float barProbability = 0.0f;
        float halfBarProbability = 0.0f;
        float quarterBarProbability = 0.0f;
        float eighthBarProbability = 0.0f;
        float sixteenthBarProbability = 0.0f;
        float thirtySecondBarProbability = 0.0f;

        // Gate setting
        float gateValue = 50.0f;       // 0-100%
        float gateRandomize = 0.0f;    // 0-100% (how much to randomize the gate)
        DirectionType gateDirection = RIGHT; // Direction of randomization

        float velocityValue = 100.0f;      // 0-100%
        float velocityRandomize = 0.0f;    // 0-100% (how much to randomize the velocity)
        DirectionType velocityDirection = RIGHT; // Direction of randomization

        float getRateValueByIdx(int idx) {
            switch (idx) {
                case RATE_1_1:
                    return barProbability;
                case RATE_1_2:
                    return halfBarProbability;
                case RATE_1_4:
                    return quarterBarProbability;
                case RATE_1_8:
                    return eighthBarProbability;
                case RATE_1_16:
                    return sixteenthBarProbability;
                case RATE_1_32:
                    return thirtySecondBarProbability;
                default:
                    return 1.0f;
            }
        }
    };

    struct MelodySettings {
        ScaleType scaleType = SCALE_MAJOR;

        int semitoneValue = 0;             // Number of semitones
        float semitoneProbability = 0.0f;  // 0-100% chance of modifying note by semitones
        bool semitoneBidirectional = false; // Whether to allow negative semitones
        DirectionType semitoneDirection = BIDIRECTIONAL; // Direction for arpeggiator

        int octaveValue = 0;            // Number of octaves
        float octaveProbability = 0.0f; // 0-100% chance of modifying note by octaves
        bool octaveBidirectional = false; // Whether to allow negative octaves
    };

    struct SamplerSettings {
        DirectionType sampleDirection = RANDOM;
        bool samplePitchFollow = false; // Whether to follow pitch changes
    };

    static inline const juce::Array<int> majorScale = {0, 2, 4, 5, 7, 9, 11};
    static inline const juce::Array<int> minorScale = {0, 2, 3, 5, 7, 8, 10};
    static inline const juce::Array<int> pentatonicScale = {0, 2, 4, 7, 9};

} // namespace MidiGeneratorParams


namespace std {
    template<>
    struct hash<Models::RateOption> {
        size_t operator()(const Models::RateOption &rate) const {
            return hash<int>()(static_cast<int>(rate));
        }
    };

    template<>
    struct hash<Models::EffectType> {
        size_t operator()(const Models::EffectType &effect) const {
            return hash<int>()(static_cast<int>(effect));
        }
    };
}