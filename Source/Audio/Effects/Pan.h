#pragma once

#include "BaseEffect.h"
#include "../../Shared/Parameters/StructParameter.h"
#include "../../Shared/Models.h"
#include "../../Shared/Parameters/Params.h"
#include "juce_dsp/juce_dsp.h"

class Pan : public BaseEffect {
public:
    Pan();
    ~Pan() override;

    void initialize(PluginProcessor &p) override;
    void prepare(const juce::dsp::ProcessSpec &spec) override;
    void process(const juce::dsp::ProcessContextReplacing<float> &context) override;
    void reset() override;

private:
    std::unique_ptr<StructParameter<Models::PanSettings>> settings;
    juce::dsp::Panner<float> pannerProcessor;
    juce::AudioParameterFloat* panParam = nullptr; // To get range info if needed
}; 