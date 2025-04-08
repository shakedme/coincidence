#include "Reverb.h"

Reverb::Reverb()
        : BaseEffect() {
}

void Reverb::initialize(PluginProcessor &p) {
    BaseEffect::initialize(p);
    std::vector<StructParameter<Models::ReverbSettings>::FieldDescriptor> descriptors = {
            makeFieldDescriptor(Params::ID_REVERB_MIX, &Models::ReverbSettings::reverbMix),
            makeFieldDescriptor(Params::ID_REVERB_TIME, &Models::ReverbSettings::reverbTime),
            makeFieldDescriptor(Params::ID_REVERB_WIDTH, &Models::ReverbSettings::reverbWidth)
    };

    settings = std::make_unique<StructParameter<Models::ReverbSettings>>(
            processor->getModulationMatrix(), descriptors);
}

void Reverb::prepare(const juce::dsp::ProcessSpec &spec) {
    BaseEffect::prepare(spec);
    reverbProcessor.prepare(spec);
    wetBuffer.setSize(spec.numChannels, spec.maximumBlockSize);
}

void Reverb::process(const juce::dsp::ProcessContextReplacing<float> &context) {
    auto &outputBlock = context.getOutputBlock();
    const int numChannels = outputBlock.getNumChannels();
    const int numSamples = outputBlock.getNumSamples();

    // Create a temporary wet buffer for reverb processing
    if (wetBuffer.getNumChannels() != numChannels || wetBuffer.getNumSamples() != numSamples) {
        wetBuffer.setSize(numChannels, numSamples);
    }

    // Copy input to wet buffer
    for (int channel = 0; channel < numChannels; ++channel) {
        const float *inputData = outputBlock.getChannelPointer(channel);
        wetBuffer.copyFrom(channel, 0, inputData, numSamples);
    }

    auto settings = this->settings->getValue();
    juce::Reverb::Parameters params;
    params.roomSize = settings.reverbTime;
    params.width = settings.reverbWidth;
    params.wetLevel = 1.0f;
    params.dryLevel = 0.0f;
    reverbProcessor.setParameters(params);

    juce::dsp::AudioBlock<float> wetBlock(wetBuffer);
    juce::dsp::ProcessContextReplacing<float> wetContext(wetBlock);
    reverbProcessor.process(wetContext);

    for (int channel = 0; channel < numChannels; ++channel) {
        float *dryData = outputBlock.getChannelPointer(channel);
        const float *wetData = wetBuffer.getReadPointer(channel);
        mixWetDrySignals(dryData, wetData, settings.reverbMix, numSamples, 1.0f);
    }
}

void Reverb::reset() {
    BaseEffect::reset();
    reverbProcessor.reset();
    wetBuffer.clear();
}