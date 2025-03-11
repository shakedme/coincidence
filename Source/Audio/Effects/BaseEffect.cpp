#include "BaseEffect.h"
#include "../Sampler/Sampler.h"

BaseEffect::BaseEffect(std::shared_ptr<TimingManager> t, SampleManager& sm, float minTimeBetweenTriggers)
    : timingManager(t)
    , sampleManager(sm)
    , MIN_TIME_BETWEEN_TRIGGERS_SECONDS(minTimeBetweenTriggers)
{
    // Initialize common properties
    sampleRate = 44100.0;
    currentBufferSize = 512;
    lastTriggerSample = 0;
}

void BaseEffect::prepareToPlay(double sampleRate, int samplesPerBlock) {
    this->sampleRate = sampleRate;
    this->currentBufferSize = samplesPerBlock;
}

void BaseEffect::releaseResources() {
    // Base implementation does nothing
}

void BaseEffect::setSettings(Params::FxSettings settings) {
    this->settings = settings;
}

bool BaseEffect::shouldApplyEffect(float probability) {
    return juce::Random::getSystemRandom().nextFloat() * 100.0f <= probability;
}

bool BaseEffect::hasMinTimePassed() {
    // Check if minimum time between triggers has passed
    juce::int64 currentSample = timingManager->getSamplePosition();
    juce::int64 minSamplesBetweenTriggers = static_cast<juce::int64>(MIN_TIME_BETWEEN_TRIGGERS_SECONDS * sampleRate);

    if (currentSample - lastTriggerSample < minSamplesBetweenTriggers)
        return false;

    return true;
}

bool BaseEffect::isEffectEnabledForSample(int effectTypeIndex) {
    int currentSampleIndex = SamplerVoice::getCurrentSampleIndex();
    auto *sound = SamplerVoice::getCorrectSoundForIndex(currentSampleIndex);
    
    if (sound != nullptr) {
        // Check if this sample belongs to a group
        int groupIndex = sound->getGroupIndex();
        if (groupIndex >= 0) {
            // Check if effect is enabled for this group
            if (!sampleManager.isGroupEffectEnabled(groupIndex, effectTypeIndex)) {
                return false;
            }
        }
        
        // Check for individual effect enablement based on effect type
        switch (effectTypeIndex) {
            case 0: // Reverb
                if (!sound->isReverbEnabled()) return false;
                break;
            case 1: // Delay
                if (!sound->isDelayEnabled()) return false;
                break;
            case 2: // Stutter
                if (!sound->isStutterEnabled()) return false;
                break;
        }
    }
    
    return true;
}

void BaseEffect::mixWetDrySignals(float* dry, const float* wet, float wetMix, int numSamples, float fadeOut) {
    for (int sample = 0; sample < numSamples; ++sample) {
        // Use equal-power crossfade to preserve perceived loudness
        float dryGain = std::cos(wetMix * juce::MathConstants<float>::halfPi);
        float wetGain = std::sin(wetMix * juce::MathConstants<float>::halfPi) * fadeOut;
        
        // Mix signals while maintaining overall volume
        dry[sample] = (dry[sample] * dryGain) + (wet[sample] * wetGain);
    }
}

void BaseEffect::applyFadeOut(float& fadeOut, float progress, float startFadePoint) {
    if (progress > startFadePoint) {
        // Use a smoother quadratic curve for the fade out
        float normalizedFade = (progress - startFadePoint) / (1.0f - startFadePoint); // 0-1 scale for the fade-out portion
        fadeOut = 1.0f - (normalizedFade * normalizedFade);
    } else {
        fadeOut = 1.0f;
    }
    
    fadeOut = juce::jlimit(0.0f, 1.0f, fadeOut);
} 