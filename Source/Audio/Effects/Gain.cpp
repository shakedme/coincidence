#include "Gain.h"

Gain::Gain()
        : BaseEffect() {
}

void Gain::initialize(PluginProcessor &p) {
    BaseEffect::initialize(p);
    gainParam = std::make_unique<Parameter<float>>(Params::ID_GAIN, processor->getModulationMatrix());
}

void Gain::prepare(const juce::dsp::ProcessSpec &spec) {
    BaseEffect::prepare(spec);
    gain.prepare(spec);
}

void Gain::process(const juce::dsp::ProcessContextReplacing<float> &context) {
    float value = gainParam->getValue();
    float gainDB = juce::jmap(value, -30.0f, 12.0f);
    gain.setGainDecibels(gainDB);
    gain.process(context);
}

void Gain::reset() {
    BaseEffect::reset();
    gain.reset();
}