#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../../Shared/Models.h"
#include "../PluginProcessor.h"
#include <vector>

class BaseEffect : public juce::dsp::ProcessorBase {
public:
    BaseEffect();

    // Initialize method to be called after default construction
    void initialize(PluginProcessor &processor);

    virtual ~BaseEffect() = default;

    virtual void prepare(const juce::dsp::ProcessSpec &spec) override;

    virtual void process(const juce::dsp::ProcessContextReplacing<float> &) override;

    virtual void reset() override;

protected:
    PluginProcessor *processorPtr = nullptr;
    TimingManager *timingManagerPtr = nullptr;

    // Common utility methods
    bool shouldApplyEffect(float probability);

    bool hasMinTimePassed();

    bool isEffectEnabledForSample(Models::EffectType effectType);

    void mixWetDrySignals(float *dry, const float *wet, float wetMix, int numSamples,
                          float fadeOut = 1.0f);

    void applyFadeOut(float &fadeOut, float progress, float startFadePoint = 0.7f);

    // Common audio properties
    double sampleRate{44100.0};
    int currentBufferSize{512};

    float MIN_TIME_BETWEEN_TRIGGERS_SECONDS = 3.0f;
    juce::int64 lastTriggerSample = 0;
}; 