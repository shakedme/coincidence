#pragma once

#include "BaseEffect.h"
#include "../../Shared/Parameters/StructParameter.h"
#include "../../Shared/Models.h"
#include "../../Shared/Parameters/Params.h"
#include "juce_dsp/juce_dsp.h"

class Compression : public BaseEffect {
public:
    Compression();

    ~Compression() override;

    void initialize(PluginProcessor &p) override;

    void prepare(const juce::dsp::ProcessSpec &spec) override;

    void process(const juce::dsp::ProcessContextReplacing<float> &context) override;

    void reset() override;

private:
    std::unique_ptr<StructParameter<Models::CompressionSettings>> settings;
    juce::dsp::Compressor<float> compressorProcessor;

    // Store pointers to parameters for range conversion
    juce::AudioParameterFloat* mixParam = nullptr;
    juce::AudioParameterFloat* thresholdParam = nullptr;
    juce::AudioParameterFloat* ratioParam = nullptr;
    juce::AudioParameterFloat* attackParam = nullptr;
    juce::AudioParameterFloat* releaseParam = nullptr;
}; 