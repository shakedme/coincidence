//
// Created by Shaked Melman on 21/03/2025.
//

#ifndef COINCIDENCE_SAMPLERVOICE_H
#define COINCIDENCE_SAMPLERVOICE_H

#include <juce_audio_utils/juce_audio_utils.h>
#include "SamplerVoiceState.h"

class SamplerVoice : public juce::SynthesiserVoice {
public:
    explicit SamplerVoice(SamplerVoiceState &state);

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

    void reset();

    [[nodiscard]] bool isVoiceActive() const override;

private:
    double pitchRatio = 1.0;
    double sourceSamplePosition = 0.0;
    double sourceEndPosition = 0.0;
    float lgain = 0.0f, rgain = 0.0f;
    bool playing = false;
    int currentSampleIndex = -1;

    SamplerVoiceState &voiceState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerVoice)
};


#endif //COINCIDENCE_SAMPLERVOICE_H
