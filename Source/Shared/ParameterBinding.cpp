//
// Created by Shaked Melman on 13/03/2025.
//

#include "ParameterBinding.h"
#include "ParameterLoader.h"

namespace AppState {


    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        juce::var parametersJson = ParameterLoader::loadParametersJson();

        if (auto *parametersArray = parametersJson.getArray()) {
            for (auto &paramData: *parametersArray) {
                ParameterLoader::addParameterFromJson(layout, paramData);
            }
        }

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
                createPercentageParam<Models::ReverbSettings>(ID_REVERB_WIDTH, &Models::ReverbSettings::reverbWidth)
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

    std::vector<ParameterDescriptor<Models::ADSRSettings>> createADSRParameters() {
        return {
                createGenericParam<Models::ADSRSettings, float>(
                        ID_ADSR_ATTACK,
                        &Models::ADSRSettings::attack,
                        [](float value) { return value * 5000.0f; }  // 0-5000ms (5 seconds)
                ),
                createGenericParam<Models::ADSRSettings, float>(
                        ID_ADSR_DECAY,
                        &Models::ADSRSettings::decay,
                        [](float value) { return value * 5000.0f; }  // 0-5000ms (5 seconds)
                ),
                createGenericParam<Models::ADSRSettings, float>(
                        ID_ADSR_SUSTAIN,
                        &Models::ADSRSettings::sustain,
                        [](float value) { return value; }            // 0.0-1.0
                ),
                createGenericParam<Models::ADSRSettings, float>(
                        ID_ADSR_RELEASE,
                        &Models::ADSRSettings::release,
                        [](float value) { return value * 5000.0f; }  // 0-5000ms (5 seconds)
                )
        };
    }

}
