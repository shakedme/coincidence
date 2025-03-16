//
// Created by Shaked Melman on 13/03/2025.
//
#pragma once

#ifndef COINCIDENCE_STATEMANAGER_H
#define COINCIDENCE_STATEMANAGER_H

#include <juce_audio_utils/juce_audio_utils.h>
#include "Models.h"
#include "ParameterBinding.h"
#include "TimingManager.h"

namespace AppState {

    // Parameter IDs
    // Rate parameters use Config::rateBaseNames directly

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
}

#endif //COINCIDENCE_STATEMANAGER_H
