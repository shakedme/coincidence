#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../../Shared/Models.h"
#include "../PluginProcessor.h"
#include <vector>

// Base class for all audio effects
class BaseEffect {
public:
    BaseEffect(PluginProcessor &processor,
               float minTimeBetweenTriggers = 3.0);

    virtual ~BaseEffect() = default;

    virtual void prepareToPlay(double sampleRate, int samplesPerBlock);

    virtual void releaseResources();

protected:
    PluginProcessor &processor;
    TimingManager &timingManager;

    // Common utility methods
    bool shouldApplyEffect(float probability);

    bool hasMinTimePassed();

    bool isEffectEnabledForSample(int effectTypeIndex);

    void mixWetDrySignals(float *dry, const float *wet, float wetMix, int numSamples,
                          float fadeOut = 1.0f);

    void applyFadeOut(float &fadeOut, float progress, float startFadePoint = 0.7f);

    // Common audio properties
    double sampleRate{44100.0};
    int currentBufferSize{512};

    float MIN_TIME_BETWEEN_TRIGGERS_SECONDS = 3.0f;
    juce::int64 lastTriggerSample = 0;
}; 