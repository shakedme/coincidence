#pragma once

#include "BaseEffect.h"
#include "../../Shared/Parameters/StructParameter.h"
#include "../../Shared/Models.h"
#include "../../Shared/Parameters/Params.h"
#include "juce_dsp/juce_dsp.h"

class Flanger : public BaseEffect {
public:
    Flanger();
    ~Flanger() override;

    void initialize(PluginProcessor &p) override;
    void prepare(const juce::dsp::ProcessSpec &spec) override;
    void process(const juce::dsp::ProcessContextReplacing<float> &context) override;
    void reset() override;

private:
    std::unique_ptr<StructParameter<Models::FlangerSettings>> settings;
    juce::dsp::Chorus<float> flangerProcessor; // JUCE's Chorus processor can be used as a flanger

    // Parameter pointers for range conversion
    juce::AudioParameterFloat* rateParam = nullptr;

    juce::AudioBuffer<float> wetBuffer;
}; 