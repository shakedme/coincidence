#include <juce_audio_utils/juce_audio_utils.h>
#include "FxEngine.h"

FxEngine::FxEngine(PluginProcessor &processorRef)
        : processor(processorRef) {
    fxChain.get<ReverbIndex>().initialize(processorRef);
    fxChain.get<DelayIndex>().initialize(processorRef);
    fxChain.get<StutterIndex>().initialize(processorRef);
}

FxEngine::~FxEngine() {
    releaseResources();
}

void FxEngine::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::dsp::ProcessSpec spec{sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2};
    fxChain.prepare(spec);
}

void FxEngine::releaseResources() {
    fxChain.reset();
}

void FxEngine::processAudio(juce::AudioBuffer<float> &buffer,
                            const juce::MidiBuffer &midiMessages) {
    auto &stutter = fxChain.get<StutterIndex>();
    stutter.setMidiMessages(midiMessages);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    fxChain.process(context);
}