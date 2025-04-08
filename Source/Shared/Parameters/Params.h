//
// Created by Shaked Melman on 02/04/2025.
//

#ifndef COINCIDENCE_APPSTATE_H
#define COINCIDENCE_APPSTATE_H

#include "juce_audio_utils/juce_audio_utils.h"

namespace Params {

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

    // Delay parameters
    static const juce::String ID_DELAY_MIX = "delay_mix";
    static const juce::String ID_DELAY_RATE = "delay_rate";
    static const juce::String ID_DELAY_FEEDBACK = "delay_feedback";
    static const juce::String ID_DELAY_PING_PONG = "delay_ping_pong";
    static const juce::String ID_DELAY_BPM_SYNC = "delay_bpm_sync";

    static const juce::Identifier ID_GAIN = "gain";
    static const juce::Identifier ID_REVERB_ENV = "reverb_envelope";

    static float percentToFloat(float percent) { return percent / 100.0f; }

    static bool toBool(float value) { return value > 0.5f; }

    static int toInt(float value) { return static_cast<int>(std::round(value)); }

    template<typename EnumType>
    static EnumType toEnum(float value) { return static_cast<EnumType>(static_cast<int>(value)); }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
}

#endif //COINCIDENCE_APPSTATE_H
