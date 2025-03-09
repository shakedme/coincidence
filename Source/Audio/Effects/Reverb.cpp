#include "Reverb.h"
#include "../Sampler/Sampler.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <random>

Reverb::Reverb(std::shared_ptr<TimingManager> t)
        : timingManager(t) {
    // Initialize JUCE reverb parameters with good default values
    juceReverbParams.roomSize = 0.8f;      // Large room size
    juceReverbParams.damping = 0.5f;       // Moderate damping
    juceReverbParams.wetLevel = 1.0f;      // Full wet signal
    juceReverbParams.dryLevel = 1.0f;      // Full dry signal (we'll handle mix ourselves)
    juceReverbParams.width = 1.0f;         // Full stereo width
    juceReverbParams.freezeMode = 0.0f;    // No freeze

    juceReverb.setParameters(juceReverbParams);

    // Initialize active reverb state
    activeReverb = {0, 0, 0, false};
}

void Reverb::setSettings(Params::FxSettings s) {
    settings = s;

    // Update JUCE reverb parameters based on our settings
    juceReverbParams.roomSize = settings.reverbTime / 100.0f;
    juceReverbParams.width = settings.reverbWidth / 100.0f;
    juceReverb.setParameters(juceReverbParams);
}

void Reverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    this->sampleRate = sampleRate;
    this->currentBufferSize = samplesPerBlock;

    // Prepare JUCE reverb
    juceReverb.setSampleRate(sampleRate);

    // Reset active reverb state
    activeReverb = {0, 0, 0, false};
}

void Reverb::releaseResources() {
    // Nothing to release for JUCE reverb
}

void Reverb::applyReverbEffect(juce::AudioBuffer<float> &buffer,
                               const std::vector<juce::int64> &triggerSamplePositions,
                               const std::vector<juce::int64> &noteDurations) {
    // Get the current sample index 
    int currentSampleIndex = SamplerVoice::getCurrentSampleIndex();

    // Check if we should apply reverb based on probability or if the sample has reverb forcibly enabled
    if (shouldApplyReverb() || shouldApplyReverbForSample(currentSampleIndex)) {
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

        float wetMix = settings.reverbMix / 100.0f;

        if (settings.reverbProbability >= 99.9f) {
            // Apply to entire buffer with proper gain preservation
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
                auto *dry = buffer.getWritePointer(channel);
                auto *wet = reverbBuffer.getReadPointer(channel);

                for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                    // Preserve gain by using equal-power crossfade
                    float dryGain = std::max(std::cos(wetMix * juce::MathConstants<float>::halfPi), 0.0f);
                    float wetGain = std::sin(wetMix * juce::MathConstants<float>::halfPi);
                    dry[sample] = dry[sample] * dryGain + wet[sample] * wetGain;
                }
            }
        } else if (!triggerSamplePositions.empty() || activeReverb.isActive) {
            // Handle ongoing reverb from previous buffer
            if (activeReverb.isActive) {
                // Calculate how much of the reverb is left in this buffer
                juce::int64 remainingDuration = activeReverb.duration - activeReverb.currentPosition;

                if (remainingDuration > 0) {
                    // Apply reverb to the remaining duration
                    int endSample = juce::jmin(buffer.getNumSamples(),
                                               static_cast<int>(remainingDuration));

                    // Apply reverb with proper gain preservation and fade-out
                    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
                        auto *dry = buffer.getWritePointer(channel);
                        auto *wet = reverbBuffer.getReadPointer(channel);

                        for (int sample = 0; sample < endSample; ++sample) {
                            // Calculate fade-out envelope - only apply to wet signal
                            float fadeOut = 1.0f - (activeReverb.currentPosition + sample) /
                                                   static_cast<float>(activeReverb.duration);
                            fadeOut = juce::jlimit(0.0f, 1.0f, fadeOut);

                            float dryGain = std::max(std::cos(wetMix * juce::MathConstants<float>::halfPi), 0.0f);
                            float wetGain = std::sin(wetMix * juce::MathConstants<float>::halfPi) * fadeOut;

                            // Properly mix signals to maintain overall volume
                            dry[sample] = dry[sample] * dryGain + wet[sample] * wetGain;
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
                // Handle new trigger position (only the first one)
            else if (!triggerSamplePositions.empty()) {
                int startSample = triggerSamplePositions[0];
                if (startSample >= 0 && startSample < buffer.getNumSamples()) {
                    juce::int64 noteDuration = (!noteDurations.empty()) ? juce::jmax(noteDurations[0],
                                                                                     static_cast<juce::int64>(
                                                                                             sampleRate * 3)) :
                                               static_cast<juce::int64>(sampleRate * 3);  // 1 second fallback

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
                            // Calculate fade-out envelope - only for wet signal
                            float fadeOut = 1.0f - (sample - startSample) /
                                                   static_cast<float>(noteDuration);
                            fadeOut = juce::jlimit(0.0f, 1.0f, fadeOut);

                            float dryGain = std::max(std::cos(wetMix * juce::MathConstants<float>::halfPi), 0.0f);
                            float wetGain = std::sin(wetMix * juce::MathConstants<float>::halfPi) * fadeOut;

                            // Properly mix signals
                            dry[sample] = dry[sample] * dryGain + wet[sample] * wetGain;
                        }
                    }

                    // Update position
                    activeReverb.currentPosition = endSample - startSample;
                }
            }
        }
    }
}

bool Reverb::shouldApplyReverb() {
    if (activeReverb.isActive || settings.reverbProbability >= 100.0f)
        return true;

    if (settings.reverbProbability <= 0.0f)
        return false;

    // Use juce::Random instead of rand() for better randomization
    return juce::Random::getSystemRandom().nextFloat() * 100.0f <= settings.reverbProbability;
}

bool Reverb::shouldApplyReverbForSample(int sampleIndex) {
    // Get the currently selected sample
    auto *sound = SamplerVoice::getCorrectSoundForIndex(sampleIndex);

    // Check if the sample exists and has reverb enabled
    if (sound != nullptr && sound->isReverbEnabled()) {
        return true;
    }

    return false;
}