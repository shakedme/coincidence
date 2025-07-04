#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../../Shared/Models.h"
#include "../../Shared/TimingManager.h"
#include "../Sampler/SampleManager.h"
#include "BaseEffect.h"
#include <vector>
#include "../PluginProcessor.h"
#include "../../Shared/Parameters/StructParameter.h"
#include "../../Shared/Parameters/Params.h"

/**
 * Delay effect processor which applies delay based on envelope value
 */
class Delay : public BaseEffect {
public:
    Delay();

    ~Delay() override;

    void initialize(PluginProcessor &p) override;

    void prepare(const juce::dsp::ProcessSpec &spec) override;

    void process(const juce::dsp::ProcessContextReplacing<float> &context) override;

    void reset() override;

private:
    std::unique_ptr<StructParameter<Models::DelaySettings>> settings;

    juce::dsp::DelayLine<float> delayLineLeft{44100};
    juce::dsp::DelayLine<float> delayLineRight{44100};
    float delayFeedback = 0.5f;
};