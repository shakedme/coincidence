#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../../Shared/Models.h"
#include "../../Shared/TimingManager.h"
#include "../Sampler/SampleManager.h"
#include "BaseEffect.h"
#include <vector>
#include "../../Shared/ParameterBinding.h"
#include "../../Gui/Components/Envelope/EnvelopeParameterMapper.h"

class PluginProcessor;

class Reverb : public BaseEffect {
public:
    // Default constructor for ProcessorChain
    Reverb();

    ~Reverb() override = default;

    // Initialize after default construction
    void initialize(PluginProcessor &p);

    // Override ProcessorBase methods
    void prepare(const juce::dsp::ProcessSpec &spec) override;

    void process(const juce::dsp::ProcessContextReplacing<float> &context) override;

    void reset() override;

    // ValueTree::Listener
//    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;

private:
    Models::ReverbSettings settings;
    std::unique_ptr<AppState::ParameterBinding<Models::ReverbSettings>> audioParamBinding;

    float envelopeValue = 1.0f;
    std::unique_ptr<AppState::SingleParameterBinding<float>> envelopeBinding;

    juce::dsp::Reverb reverbProcessor;
    juce::AudioBuffer<float> wetBuffer;
};
