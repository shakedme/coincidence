#pragma once

#include "BaseEffect.h"
#include "../../Shared/Parameters/StructParameter.h"
#include "../../Shared/Models.h"
#include "../../Shared/Parameters/Params.h"
#include "juce_dsp/juce_dsp.h"

class Phaser : public BaseEffect {
public:
    Phaser();
    ~Phaser() override;

    void initialize(PluginProcessor &p) override;
    void prepare(const juce::dsp::ProcessSpec &spec) override;
    void process(const juce::dsp::ProcessContextReplacing<float> &context) override;
    void reset() override;

private:
    std::unique_ptr<StructParameter<Models::PhaserSettings>> settings;
    juce::dsp::Phaser<float> phaserProcessor;

    juce::AudioParameterFloat* rateParam = nullptr;

    juce::AudioBuffer<float> wetBuffer;
}; 