#include "Gain.h"

Gain::Gain()
        : BaseEffect() {
}

void Gain::initialize(PluginProcessor &p) {
    BaseEffect::initialize(p);
    envelopeBinding = std::make_unique<AppState::SingleParameterBinding<float>>(envelopeValue, p.getAPVTS().state,
                                                                                AppState::ID_AMPLITUDE_ENVELOPE);
}

void Gain::prepare(const juce::dsp::ProcessSpec &spec) {
    BaseEffect::prepare(spec);
    gain.prepare(spec);
    gain.setGainLinear(envelopeValue);
}

void Gain::process(const juce::dsp::ProcessContextReplacing<float> &context) {
    if (envelopeValue < 0.001f) {
        return;
    }
    gain.setGainLinear(envelopeValue);
    gain.process(context);
}

void Gain::reset() {
    BaseEffect::reset();
    gain.reset();
}