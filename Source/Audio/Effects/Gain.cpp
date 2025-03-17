#include "Gain.h"

Gain::Gain()
        : BaseEffect() {
}

void Gain::initialize(PluginProcessor &p) {
    BaseEffect::initialize(p);

    // Store a reference to the envelope manager
    envelopeManagerPtr = &p.getEnvelopeManager();

    // Get the amplitude envelope mapper
    if (envelopeManagerPtr != nullptr) {
        amplitudeEnvelopeMapper = envelopeManagerPtr->getMapper(EnvelopeParams::ParameterType::Amplitude);
    }
}

void Gain::prepare(const juce::dsp::ProcessSpec &spec) {
    BaseEffect::prepare(spec);
}

void Gain::process(const juce::dsp::ProcessContextReplacing<float> &context) {
    // Skip processing if we don't have a valid amplitude envelope mapper
    if (amplitudeEnvelopeMapper == nullptr) {
        return;
    }

    auto &outputBlock = context.getOutputBlock();
    const int numChannels = outputBlock.getNumChannels();
    const int numSamples = outputBlock.getNumSamples();

    // Apply envelope to each sample in all channels
    for (int sample = 0; sample < numSamples; ++sample) {
        // Get current envelope value
        float envelopeValue = amplitudeEnvelopeMapper->getCurrentValue();

        // Apply to all channels
        for (int channel = 0; channel < numChannels; ++channel) {
            float *channelData = outputBlock.getChannelPointer(channel);
            channelData[sample] *= envelopeValue;
        }
    }
}

void Gain::reset() {
    BaseEffect::reset();
} 