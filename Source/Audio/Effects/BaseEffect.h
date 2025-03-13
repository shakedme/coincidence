#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../../Shared/Models.h"
#include "../../Shared/TimingManager.h"
#include "../Sampler/SampleManager.h"
#include <vector>

// Base class for all audio effects
class BaseEffect {
public:
    BaseEffect(std::shared_ptr<TimingManager> t, SampleManager &sm,
               float minTimeBetweenTriggers = 3.0);

    virtual ~BaseEffect() = default;

    virtual void prepareToPlay(double sampleRate, int samplesPerBlock);

    virtual void releaseResources();

    virtual void setSettings(Config::FxSettings settings);

protected:

    // Common utility methods
    bool shouldApplyEffect(float probability);

    bool hasMinTimePassed();

    bool isEffectEnabledForSample(int effectTypeIndex);

    void mixWetDrySignals(float *dry, const float *wet, float wetMix, int numSamples,
                          float fadeOut = 1.0f);

    void applyFadeOut(float &fadeOut, float progress, float startFadePoint = 0.7f);

    // Shared resources
    std::shared_ptr<TimingManager> timingManager;
    SampleManager &sampleManager;
    Config::FxSettings settings;

    // Common audio properties
    double sampleRate{44100.0};
    int currentBufferSize{512};

    float MIN_TIME_BETWEEN_TRIGGERS_SECONDS = 3.0f;
    juce::int64 lastTriggerSample = 0;
}; 