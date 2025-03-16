#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>
#include <vector>
#include "../../Shared/Models.h"
#include "Reverb.h"
#include "Delay.h"
#include "Stutter.h"
#include "BaseEffect.h"

// Forward declarations
class PluginProcessor;

class FxEngine {
public:
    FxEngine(PluginProcessor &processorRef);

    ~FxEngine();

    void prepareToPlay(double sampleRate, int samplesPerBlock);

    void releaseResources();

    void processAudio(juce::AudioBuffer<float> &buffer,
                      const juce::MidiBuffer &midiMessages);

private:
    PluginProcessor &processor;

    // Audio effect objects
    std::unique_ptr<Reverb> reverbEffect;
    std::unique_ptr<Delay> delayEffect;
    std::unique_ptr<Stutter> stutterEffect;

    // Audio settings
    double sampleRate{44100.0};
    int bufferSize{512};

    std::vector<juce::int64> getNoteDurations(const std::vector<juce::int64> &triggerPositions);

    std::vector<juce::int64> checkForMidiTriggers(const juce::MidiBuffer &midiMessages);

};