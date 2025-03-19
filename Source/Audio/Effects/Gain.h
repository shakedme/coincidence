#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../../Shared/Models.h"
#include "../../Shared/ParameterBinding.h"
#include "BaseEffect.h"
#include <vector>
#include <memory>

/**
 * Gain processor that applies amplitude envelope modulation
 */
class Gain : public BaseEffect {
public:
    Gain();

    ~Gain() override = default;

    // Initialize after default construction
    void initialize(PluginProcessor &p);

    // Override ProcessorBase methods
    void prepare(const juce::dsp::ProcessSpec &spec) override;

    void process(const juce::dsp::ProcessContextReplacing<float> &context) override;

    void reset() override;

private:
    float envelopeValue = 1.0f;
    juce::dsp::Gain<float> gain;
    std::unique_ptr<AppState::SingleParameterBinding<float>> paramBinding;
}; 