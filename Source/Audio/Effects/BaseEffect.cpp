#include "BaseEffect.h"
#include "../Sampler/Sampler.h"

BaseEffect::BaseEffect() {
    sampleRate = 44100.0;
    currentBufferSize = 512;
    lastTriggerSample = 0;
}

void BaseEffect::initialize(PluginProcessor &processorToUse) {
    processorPtr = &processorToUse;
    timingManagerPtr = &processorToUse.getTimingManager();
}

void BaseEffect::prepare(const juce::dsp::ProcessSpec &spec) {
    // Store basic information from the spec
    sampleRate = spec.sampleRate;
    currentBufferSize = static_cast<int>(spec.maximumBlockSize);

    // Reset state when preparing
    reset();
}

void BaseEffect::process(const juce::dsp::ProcessContextReplacing<float> &context) {
    // Base implementation just passes audio through
    // Subclasses should override this to actually process audio
}

void BaseEffect::reset() {
    // Reset any internal state
    lastTriggerSample = 0;
}

bool BaseEffect::shouldApplyEffect(float probability) {
    return juce::Random::getSystemRandom().nextFloat() <= probability;
}

bool BaseEffect::hasMinTimePassed() {
    // Check if minimum time between triggers has passed
    if (!processorPtr || !timingManagerPtr) return false;

    juce::int64 currentSample = timingManagerPtr->getSamplePosition();
    juce::int64 minSamplesBetweenTriggers = static_cast<juce::int64>(MIN_TIME_BETWEEN_TRIGGERS_SECONDS * sampleRate);

    if (currentSample - lastTriggerSample < minSamplesBetweenTriggers)
        return false;

    return true;
}

bool BaseEffect::isEffectEnabledForSample(Models::EffectType effectType) {
    if (!processorPtr) return false;

    int currentSampleIndex = processorPtr->getSampleManager().getCurrentSampleIndex();
    auto *sound = processorPtr->getSampleManager().getCorrectSoundForIndex(currentSampleIndex);

    if (sound != nullptr) {
        // Check if this sample belongs to a group
        int groupIndex = sound->getGroupIndex();
        if (groupIndex >= 0) {
            // Check if effect is enabled for this group
            if (!processorPtr->getSampleManager().isGroupEffectEnabled(groupIndex, effectType)) {
                return false;
            }
        }

        // Check for individual effect enablement based on effect type
        switch (effectType) {
            case Models::EffectType::REVERB:
                if (!sound->isReverbEnabled()) return false;
                break;
            case Models::EffectType::DELAY:
                if (!sound->isDelayEnabled()) return false;
                break;
            case Models::EffectType::STUTTER:
                if (!sound->isStutterEnabled()) return false;
                break;
        }
    }

    return true;
}

void BaseEffect::mixWetDrySignals(float *dry, const float *wet, float wetMix, int numSamples, float fadeOut) {
    for (int sample = 0; sample < numSamples; ++sample) {
        // Use equal-power crossfade to preserve perceived loudness
        float dryGain = std::cos(wetMix * juce::MathConstants<float>::halfPi);
        float wetGain = std::sin(wetMix * juce::MathConstants<float>::halfPi) * fadeOut;

        // Mix signals while maintaining overall volume
        dry[sample] = (dry[sample] * dryGain) + (wet[sample] * wetGain);
    }
}

void BaseEffect::applyFadeOut(float &fadeOut, float progress, float startFadePoint) {
    if (progress > startFadePoint) {
        // Use a smoother quadratic curve for the fade out
        float normalizedFade =
                (progress - startFadePoint) / (1.0f - startFadePoint); // 0-1 scale for the fade-out portion
        fadeOut = 1.0f - (normalizedFade * normalizedFade);
    } else {
        fadeOut = 1.0f;
    }

    fadeOut = juce::jlimit(0.0f, 1.0f, fadeOut);
}
