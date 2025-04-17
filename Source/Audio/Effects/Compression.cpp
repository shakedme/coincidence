#include "Compression.h"

Compression::Compression()
        : BaseEffect() {
}

Compression::~Compression() {
    // Cleanup resources if needed
}

void Compression::initialize(PluginProcessor &p) {
    BaseEffect::initialize(p);

    std::vector<StructParameter<Models::CompressionSettings>::FieldDescriptor> descriptors = {
            makeFieldDescriptor(Params::ID_COMPRESSION_MIX, &Models::CompressionSettings::mix),
            makeFieldDescriptor(Params::ID_COMPRESSION_THRESHOLD, &Models::CompressionSettings::threshold),
            makeFieldDescriptor(Params::ID_COMPRESSION_RATIO, &Models::CompressionSettings::ratio),
            makeFieldDescriptor(Params::ID_COMPRESSION_ATTACK, &Models::CompressionSettings::attack),
            makeFieldDescriptor(Params::ID_COMPRESSION_RELEASE, &Models::CompressionSettings::release)
    };

    settings = std::make_unique<StructParameter<Models::CompressionSettings>>(
            processor->getModulationMatrix(), descriptors);

    auto& apvts = processor->getAPVTS();
    thresholdParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(Params::ID_COMPRESSION_THRESHOLD));
    ratioParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(Params::ID_COMPRESSION_RATIO));
    attackParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(Params::ID_COMPRESSION_ATTACK));
    releaseParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(Params::ID_COMPRESSION_RELEASE));
}

void Compression::prepare(const juce::dsp::ProcessSpec &spec) {
    BaseEffect::prepare(spec);
    compressorProcessor.prepare(spec);
    reset();
}

void Compression::process(const juce::dsp::ProcessContextReplacing<float> &context) {
    auto settings = this->settings->getValue(); // Settings now contain normalized 0-1 values

    if (settings.mix > 0.001f) {
        float actualThreshold = thresholdParam->convertFrom0to1(settings.threshold);
        float actualRatio = ratioParam->convertFrom0to1(settings.ratio);
        float actualAttack = attackParam->convertFrom0to1(settings.attack);
        float actualRelease = releaseParam->convertFrom0to1(settings.release);

        compressorProcessor.setThreshold(actualThreshold);
        compressorProcessor.setRatio(actualRatio);
        compressorProcessor.setAttack(actualAttack);
        compressorProcessor.setRelease(actualRelease);

        auto &outputBlock = context.getOutputBlock();
        juce::AudioBuffer<float> wetBuffer;
        wetBuffer.setSize(outputBlock.getNumChannels(), outputBlock.getNumSamples(), false, false,
                          true);

        juce::dsp::AudioBlock<float> wetBlock(wetBuffer);
        wetBlock.copyFrom(outputBlock);

        juce::dsp::ProcessContextReplacing<float> wetContext(wetBlock);

        compressorProcessor.process(wetContext);

        for (int channel = 0; channel < outputBlock.getNumChannels(); ++channel) {
            float *dryData = outputBlock.getChannelPointer(channel);
            const float *wetData = wetBuffer.getReadPointer(channel);
            // Use the normalized mix value directly from settings
            mixWetDrySignals(dryData, wetData, settings.mix, outputBlock.getNumSamples(), 1.0f);
        }
    }
}

void Compression::reset() {
    BaseEffect::reset();
    compressorProcessor.reset();
} 