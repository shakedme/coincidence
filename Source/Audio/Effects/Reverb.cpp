#include "Reverb.h"

Reverb::Reverb()
        : BaseEffect() {
}

void Reverb::initialize(PluginProcessor &p) {
    BaseEffect::initialize(p);

    paramBinding = AppState::createParameterBinding<Models::ReverbSettings>(settings, p.getAPVTS());
    paramBinding->registerParameters(AppState::createReverbParameters());
}

void Reverb::prepare(const juce::dsp::ProcessSpec &spec) {
    BaseEffect::prepare(spec);
    reverbProcessor.prepare(spec);
}

void Reverb::process(const juce::dsp::ProcessContextReplacing<float> &context) {
    if (settings.reverbMix > 0.01f) {
        juce::Reverb::Parameters params;
        params.roomSize = settings.reverbTime;
        params.width = settings.reverbWidth;
        params.wetLevel = settings.reverbMix;
        params.dryLevel = 1.0f - settings.reverbMix;

        reverbProcessor.setParameters(params);
        reverbProcessor.process(context);
    }
}

void Reverb::reset() {
    BaseEffect::reset();
    reverbProcessor.reset();
}