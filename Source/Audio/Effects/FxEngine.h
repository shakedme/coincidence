#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../Shared/TimingManager.h"
#include "Stutter.h"
#include "Reverb.h"

class FxEngine
{
public:
    FxEngine(std::shared_ptr<TimingManager> timingManager);
    ~FxEngine();

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();
    void processAudio(juce::AudioBuffer<float>& buffer,
                      juce::AudioPlayHead* playHead,
                      const juce::MidiBuffer& midiMessages);
    void setSettings(Params::FxSettings s);

private:
    std::unique_ptr<Stutter> stutterEffect;
    std::unique_ptr<Reverb> reverbEffect;
    std::shared_ptr<TimingManager> timingManager;

    Params::FxSettings settings;
    double sampleRate {44100.0};
    int bufferSize {512};

    void updateTimingInfo(juce::AudioPlayHead* playHead);
    void updateFxWithBufferSize(int numSamples);
    std::vector<int> checkForMidiTriggers(const juce::MidiBuffer& midiMessages);
};