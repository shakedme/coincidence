#include "Flanger.h"

Flanger::Flanger()
        : BaseEffect() {
}

Flanger::~Flanger() {
}

void Flanger::initialize(PluginProcessor &p) {
    BaseEffect::initialize(p);

    std::vector<StructParameter<Models::FlangerSettings>::FieldDescriptor> descriptors = {
            makeFieldDescriptor(Params::ID_FLANGER_MIX, &Models::FlangerSettings::mix),
            makeFieldDescriptor(Params::ID_FLANGER_RATE, &Models::FlangerSettings::rate),
            makeFieldDescriptor(Params::ID_FLANGER_DEPTH, &Models::FlangerSettings::depth),
            makeFieldDescriptor(Params::ID_FLANGER_FEEDBACK, &Models::FlangerSettings::feedback)
    };

    settings = std::make_unique<StructParameter<Models::FlangerSettings>>(
            processor->getModulationMatrix(), descriptors);

    auto &apvts = processor->getAPVTS();
    rateParam = dynamic_cast<juce::AudioParameterFloat *>(apvts.getParameter(Params::ID_FLANGER_RATE));
}

void Flanger::prepare(const juce::dsp::ProcessSpec &spec) {
    BaseEffect::prepare(spec);
    flangerProcessor.prepare(spec);
    wetBuffer.setSize(spec.numChannels, spec.maximumBlockSize);
    reset();
}

void Flanger::process(const juce::dsp::ProcessContextReplacing<float> &context) {
    auto settings = this->settings->getValue();

    if (settings.mix < 0.001f)
        return;

    flangerProcessor.setRate(rateParam->convertFrom0to1(settings.rate));
    flangerProcessor.setDepth(settings.depth);
    flangerProcessor.setFeedback(settings.feedback);
    flangerProcessor.setCentreDelay(1.5f);
    flangerProcessor.setMix(1);

    auto &outputBlock = context.getOutputBlock();
    wetBuffer.setSize(outputBlock.getNumChannels(), outputBlock.getNumSamples(), false, false, true);

    juce::dsp::AudioBlock<float> wetBlock(wetBuffer);
    wetBlock.copyFrom(outputBlock);

    juce::dsp::ProcessContextReplacing<float> wetContext(wetBlock);

    flangerProcessor.process(wetContext);

    for (int channel = 0; channel < outputBlock.getNumChannels(); ++channel) {
        float *dryData = outputBlock.getChannelPointer(channel);
        const float *wetData = wetBuffer.getReadPointer(channel);
        mixWetDrySignals(dryData, wetData, settings.mix, outputBlock.getNumSamples(), 1.0f);
    }
}

void Flanger::reset() {
    BaseEffect::reset();
    flangerProcessor.reset();
    wetBuffer.clear();
} 