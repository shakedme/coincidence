#include "Reverb.h"

Reverb::Reverb()
        : BaseEffect() {
}

void Reverb::initialize(PluginProcessor &p) {
    BaseEffect::initialize(p);
    paramBinding = AppState::createParameterBinding<Models::ReverbSettings>(settings, p.getAPVTS());
    paramBinding->registerParameters(AppState::createReverbParameters());
    envelopeManagerPtr = &p.getEnvelopeManager();

    juce::Timer::callAfterDelay(2000, [this]() {
        reverbEnvelopeMapper = envelopeManagerPtr->getMapper(EnvelopeParams::ParameterType::Reverb);
    });
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

    // Configure reverb parameters (100% wet for the reverb processor)
    juce::Reverb::Parameters params;
    params.roomSize = settings.reverbTime;
    params.width = settings.reverbWidth;
    params.wetLevel = 1.0f;  // Always 100% wet for the reverb signal
    params.dryLevel = 0.0f;  // No dry in the wet buffer
    reverbProcessor.setParameters(params);

    // Process the wet buffer through reverb
    juce::dsp::AudioBlock<float> wetBlock(wetBuffer);
    juce::dsp::ProcessContextReplacing<float> wetContext(wetBlock);
    reverbProcessor.process(wetContext);

    // Calculate the mix value with envelope modulation
    float baseMix = settings.reverbMix;
    float mixValue = baseMix;

    // Apply envelope modulation to the mix if we have a valid envelope mapper
    if (reverbEnvelopeMapper != nullptr) {
        float envelopeValue = reverbEnvelopeMapper->getCurrentValue();
        mixValue = baseMix * envelopeValue;
    }

    // Mix wet and dry signals using the BaseEffect helper method
    for (int channel = 0; channel < numChannels; ++channel) {
        float *dryData = outputBlock.getChannelPointer(channel);
        const float *wetData = wetBuffer.getReadPointer(channel);

        // Use the mix helper with fadeOut = 1.0f (no fade out)
        mixWetDrySignals(dryData, wetData, mixValue, numSamples, 1.0f);
    }
}

void Reverb::reset() {
    BaseEffect::reset();
    reverbProcessor.reset();
    wetBuffer.clear();
}