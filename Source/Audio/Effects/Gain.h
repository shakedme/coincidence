#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../../Shared/Models.h"
#include "BaseEffect.h"
#include "../Envelope/EnvelopeManager.h"
#include <vector>

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
    EnvelopeManager* envelopeManagerPtr = nullptr;
    EnvelopeParameterMapper* amplitudeEnvelopeMapper = nullptr;
}; 