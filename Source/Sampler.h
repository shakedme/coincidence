//
// Created by Shaked Melman on 01/03/2025.
//

#ifndef JUCECMAKEREPO_SAMPLERSOUND_H
#define JUCECMAKEREPO_SAMPLERSOUND_H

#include "juce_audio_utils/juce_audio_utils.h"
#include <juce_audio_basics/juce_audio_basics.h>

class SamplerSound : public juce::SynthesiserSound
{
public:
    SamplerSound(const juce::String& name,
                 juce::AudioFormatReader& source,
                 const juce::BigInteger& notes);

    // SynthesiserSound methods
    bool appliesToNote(int midiNoteNumber) override;
    bool appliesToChannel(int midiChannel) override;

    // Getters for sample data
    juce::AudioBuffer<float>* getAudioData() { return &audioData; }
    double getSourceSampleRate() const { return sourceSampleRate; }

private:
    juce::String name;
    juce::AudioBuffer<float> audioData;
    juce::BigInteger midiNotes;
    double sourceSampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerSound)
};

class SamplerVoice : public juce::SynthesiserVoice
{
public:
    SamplerVoice();

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                         int startSample,
                         int numSamples) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;

private:
    double pitchRatio = 1.0;
    double sourceSamplePosition = 0.0;
    float lgain = 0.0f, rgain = 0.0f;
    bool playing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerVoice)
};

#endif //JUCECMAKEREPO_SAMPLERSOUND_H
