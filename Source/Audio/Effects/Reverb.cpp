#include "Reverb.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <random>

Reverb::Reverb(PluginProcessor &p)
        : BaseEffect(p, 3.0) // 3.0 seconds between triggers
{
    paramBinding = AppState::createParameterBinding<Models::ReverbSettings>(settings, processor.getAPVTS());
    paramBinding->registerParameters(AppState::createReverbParameters());

    // Initialize JUCE reverb parameters with good default values
    juceReverbParams.roomSize = 0.8f;      // Large room size
    juceReverbParams.damping = 0.5f;       // Moderate damping
    juceReverbParams.wetLevel = 1.0f;      // Full wet signal
    juceReverbParams.dryLevel = 0.0f;      // No dry signal - we'll handle mix ourselves
    juceReverbParams.width = 1.0f;         // Full stereo width
    juceReverbParams.freezeMode = 0.0f;    // No freeze

    // Initialize active reverb state
    activeReverb = {};

    // Start the timer for updating reverb parameters
    startTimer(1000);
}

void Reverb::timerCallback() {
    juceReverbParams.roomSize = settings.reverbTime;
    juceReverbParams.width = settings.reverbWidth;
    juceReverb.setParameters(juceReverbParams);
}

void Reverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    BaseEffect::prepareToPlay(sampleRate, samplesPerBlock);

    // Prepare JUCE reverb
    juceReverb.setSampleRate(sampleRate);

    // Reset active reverb state
    activeReverb = {};
}

void Reverb::releaseResources() {
    BaseEffect::releaseResources();
    // Nothing to release for JUCE reverb
}

void Reverb::applyReverbEffect(juce::AudioBuffer<float> &buffer,
                               const std::vector<juce::int64> &triggerSamplePositions) {

    // Check if we should apply reverb based on probability
    if (shouldApplyReverb()) {
        // Create a temporary buffer for the wet signal
        juce::AudioBuffer<float> reverbBuffer;
        reverbBuffer.makeCopyOf(buffer);

        // Process with JUCE's reverb
        if (reverbBuffer.getNumChannels() == 2) {
            juceReverb.processStereo(reverbBuffer.getWritePointer(0),
                                     reverbBuffer.getWritePointer(1),
                                     reverbBuffer.getNumSamples());
        } else {
            juceReverb.processMono(reverbBuffer.getWritePointer(0),
                                   reverbBuffer.getNumSamples());
        }

        float wetMix = settings.reverbMix;

        if (settings.reverbProbability >= 99.9f) {
            // Apply to entire buffer
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
                mixWetDrySignals(buffer.getWritePointer(channel),
                                 reverbBuffer.getReadPointer(channel),
                                 wetMix,
                                 buffer.getNumSamples());
            }
        } else if (!triggerSamplePositions.empty() || activeReverb.isActive) {
            // Handle ongoing reverb
            if (activeReverb.isActive) {
                processActiveReverb(buffer, reverbBuffer, wetMix);
            }
                // Handle new trigger
            else if (!triggerSamplePositions.empty() && isReverbEnabledForSample() &&
                     hasMinTimePassed()) {
                processNewReverbTrigger(buffer, reverbBuffer, triggerSamplePositions, wetMix);
            }
        }
    }
}

void Reverb::processActiveReverb(juce::AudioBuffer<float> &buffer,
                                 const juce::AudioBuffer<float> &reverbBuffer,
                                 float wetMix) {
    // Calculate how much of the reverb is left in this buffer
    juce::int64 remainingDuration = activeReverb.duration - activeReverb.currentPosition;

    if (remainingDuration > 0) {
        // Apply reverb to the remaining duration
        int endSample = juce::jmin(buffer.getNumSamples(), static_cast<int>(remainingDuration));

        // Apply reverb with proper gain preservation
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            auto *dry = buffer.getWritePointer(channel);
            auto *wet = reverbBuffer.getReadPointer(channel);

            for (int sample = 0; sample < endSample; ++sample) {
                // Calculate fade-out envelope
                float progress = (activeReverb.currentPosition + sample) / static_cast<float>(activeReverb.duration);
                float fadeOut = 1.0f;
                applyFadeOut(fadeOut, progress, 0.7f);

                // Mix signals with the calculated fade
                float dryGain = std::cos(wetMix * juce::MathConstants<float>::halfPi);
                float wetGain = std::sin(wetMix * juce::MathConstants<float>::halfPi) * fadeOut;
                dry[sample] = (dry[sample] * dryGain) + (wet[sample] * wetGain);
            }
        }

        // Update position
        activeReverb.currentPosition += endSample;

        // Check if reverb is complete
        if (activeReverb.currentPosition >= activeReverb.duration) {
            activeReverb.isActive = false;
        }
    }
}

void Reverb::processNewReverbTrigger(juce::AudioBuffer<float> &buffer,
                                     const juce::AudioBuffer<float> &reverbBuffer,
                                     const std::vector<juce::int64> &triggerSamplePositions,
                                     float wetMix) {
    int startSample = triggerSamplePositions[0];
    if (startSample >= 0 && startSample < buffer.getNumSamples()) {
        // Update the last trigger time
        lastTriggerSample = processor.getTimingManager().getSamplePosition() + startSample;

        juce::int64 noteDuration = static_cast<juce::int64>(sampleRate * 3.0);

        // Start a new reverb effect
        activeReverb = {
                startSample,
                noteDuration,
                0,
                true
        };

        // Apply reverb to the note region
        int endSample = juce::jmin(buffer.getNumSamples(),
                                   startSample + static_cast<int>(noteDuration));

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            auto *dry = buffer.getWritePointer(channel);
            auto *wet = reverbBuffer.getReadPointer(channel);

            for (int sample = startSample; sample < endSample; ++sample) {
                float dryGain = std::cos(wetMix * juce::MathConstants<float>::halfPi);
                float wetGain = std::sin(wetMix * juce::MathConstants<float>::halfPi);

                // Properly mix signals
                dry[sample] = (dry[sample] * dryGain) + (wet[sample] * wetGain);
            }
        }

        // Update position
        activeReverb.currentPosition = endSample - startSample;
    }
}

bool Reverb::shouldApplyReverb() {
    if (activeReverb.isActive) {
        return true;
    }
    // Apply random probability using the base class method
    return BaseEffect::shouldApplyEffect(settings.reverbProbability);
}

bool Reverb::isReverbEnabledForSample() {
    // Use the base class method with reverb effect type (0)
    return BaseEffect::isEffectEnabledForSample(0);
}