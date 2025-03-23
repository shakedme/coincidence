//
// Created by Shaked Melman on 21/03/2025.
//

#ifndef COINCIDENCE_SAMPLERVOICE_H
#define COINCIDENCE_SAMPLERVOICE_H

#include <juce_audio_utils/juce_audio_utils.h>
#include "SamplerVoiceState.h"

class SamplerVoice : public juce::SynthesiserVoice {
public:
    SamplerVoice();

    void setVoiceState(SamplerVoiceState *state) { voiceState = state; }

    bool canPlaySound(juce::SynthesiserSound *sound) override;

    void renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                         int startSample,
                         int numSamples) override;

    void startNote(int midiNoteNumber,
                   float velocity,
                   juce::SynthesiserSound *sound,
                   int currentPitchWheelPosition) override;

    void stopNote(float velocity, bool allowTailOff) override;

    void pitchWheelMoved(int newPitchWheelValue) override;

    void controllerMoved(int controllerNumber, int newControllerValue) override;

    // Reset all voice state
    void reset();

    // Helper method to check if voice is active
    [[nodiscard]] bool isVoiceActive() const override;

    // Update the ADSR parameters for this voice
    void updateADSRParameters(const juce::ADSR::Parameters &newParams) {
        adsr.setParameters(newParams);
    }

    // Set maximum playback duration in samples
    void setMaxPlayDuration(juce::int64 durationInSamples) {
        maxPlayDuration = durationInSamples;
        sampleCounter = 0;
    }

private:
    double pitchRatio = 1.0;
    double sourceSamplePosition = 0.0;
    double sourceEndPosition = 0.0;
    float lgain = 0.0f, rgain = 0.0f;
    bool playing = false;
    int currentSampleIndex = -1;

    // Maximum playback duration in samples (when > 0)
    juce::int64 maxPlayDuration = 0;
    juce::int64 sampleCounter = 0;

    // ADSR envelope processor
    juce::ADSR adsr;

    SamplerVoiceState *voiceState = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerVoice)
};


#endif //COINCIDENCE_SAMPLERVOICE_H
