#include "Pan.h"

Pan::Pan()
        : BaseEffect() {
}

Pan::~Pan() {
}

void Pan::initialize(PluginProcessor &p) {
    BaseEffect::initialize(p);

    std::vector<StructParameter<Models::PanSettings>::FieldDescriptor> descriptors = {
            makeFieldDescriptor(Params::ID_PAN, &Models::PanSettings::panPosition)
    };

    settings = std::make_unique<StructParameter<Models::PanSettings>>(
            processor->getModulationMatrix(), descriptors);

    auto& apvts = processor->getAPVTS();
    panParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(Params::ID_PAN));
    jassert (panParam);
}

void Pan::prepare(const juce::dsp::ProcessSpec &spec) {
    BaseEffect::prepare(spec);
    pannerProcessor.prepare(spec);
    pannerProcessor.setRule(juce::dsp::PannerRule::linear); // Or constantPower, etc.
    reset();
}

void Pan::process(const juce::dsp::ProcessContextReplacing<float> &context) {
    auto settings = this->settings->getValue(); // panPosition is normalized 0-1

    // Convert normalized 0-1 to the panner's expected -1 to 1 range
    float panValue = juce::jmap(settings.panPosition, 0.0f, 1.0f, -1.0f, 1.0f);

    pannerProcessor.setPan(panValue);
    pannerProcessor.process(context);
}

void Pan::reset() {
    BaseEffect::reset();
    pannerProcessor.reset();
} 