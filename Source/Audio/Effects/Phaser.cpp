#include "Phaser.h"

Phaser::Phaser()
        : BaseEffect() {
}

Phaser::~Phaser() {
}

void Phaser::initialize(PluginProcessor &p) {
    BaseEffect::initialize(p);

    std::vector<StructParameter<Models::PhaserSettings>::FieldDescriptor> descriptors = {
            makeFieldDescriptor(Params::ID_PHASER_MIX, &Models::PhaserSettings::mix),
            makeFieldDescriptor(Params::ID_PHASER_RATE, &Models::PhaserSettings::rate),
            makeFieldDescriptor(Params::ID_PHASER_DEPTH, &Models::PhaserSettings::depth),
            makeFieldDescriptor(Params::ID_PHASER_FEEDBACK, &Models::PhaserSettings::feedback),
            makeFieldDescriptor(Params::ID_PHASER_STAGES, &Models::PhaserSettings::stages)
    };

    settings = std::make_unique<StructParameter<Models::PhaserSettings>>(
            processor->getModulationMatrix(), descriptors);

    auto &apvts = processor->getAPVTS();
    rateParam = dynamic_cast<juce::AudioParameterFloat *>(apvts.getParameter(Params::ID_PHASER_RATE));
}

void Phaser::prepare(const juce::dsp::ProcessSpec &spec) {
    BaseEffect::prepare(spec);
    phaserProcessor.prepare(spec);
    wetBuffer.setSize(spec.numChannels, spec.maximumBlockSize);
    reset();
}

void Phaser::process(const juce::dsp::ProcessContextReplacing<float> &context) {
    auto settings = this->settings->getValue();

    if (settings.mix < 0.001f)
        return;

    float actualRate = rateParam->convertFrom0to1(settings.rate);

    phaserProcessor.setRate(actualRate);
    phaserProcessor.setDepth(settings.depth);
    phaserProcessor.setFeedback(settings.feedback);
    phaserProcessor.setMix(1);

    auto &outputBlock = context.getOutputBlock();
    wetBuffer.setSize(outputBlock.getNumChannels(), outputBlock.getNumSamples(), false, false, true);

    juce::dsp::AudioBlock<float> wetBlock(wetBuffer);
    wetBlock.copyFrom(outputBlock);

    juce::dsp::ProcessContextReplacing<float> wetContext(wetBlock);

    phaserProcessor.process(wetContext);

    for (int channel = 0; channel < outputBlock.getNumChannels(); ++channel) {
        float *dryData = outputBlock.getChannelPointer(channel);
        const float *wetData = wetBuffer.getReadPointer(channel);
        mixWetDrySignals(dryData, wetData, settings.mix, outputBlock.getNumSamples(), 1.0f);
    }
}

void Phaser::reset() {
    BaseEffect::reset();
    phaserProcessor.reset();
    wetBuffer.clear();
} 