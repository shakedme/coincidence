#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../../Shared/Models.h"
#include "../../Shared/TimingManager.h"
#include "../Sampler/SampleManager.h"
#include "BaseEffect.h"
#include <vector>
#include "../../Shared/Parameters/Params.h"
#include "../../Shared/Parameters/StructParameter.h"

class PluginProcessor;

class Reverb : public BaseEffect {
public:
    Reverb();

    ~Reverb() override = default;

    void initialize(PluginProcessor &p) override;

    void prepare(const juce::dsp::ProcessSpec &spec) override;

    void process(const juce::dsp::ProcessContextReplacing<float> &context) override;

    void reset() override;

private:
    std::unique_ptr<StructParameter<Models::ReverbSettings>> settings;

    juce::dsp::Reverb reverbProcessor;
    juce::AudioBuffer<float> wetBuffer;
};
