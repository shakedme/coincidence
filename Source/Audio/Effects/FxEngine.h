#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <vector>
#include "../../Shared/Models.h"
#include "Reverb.h"
#include "Delay.h"
#include "Stutter.h"
#include "BaseEffect.h"

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

    enum {
        ReverbIndex,
        DelayIndex,
        StutterIndex
    };
    juce::dsp::ProcessorChain<Reverb, Delay, Stutter> fxChain;
};