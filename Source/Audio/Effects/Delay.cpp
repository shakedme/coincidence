#include "Delay.h"
#include <random>

Delay::Delay(PluginProcessor &p)
        : BaseEffect(p, 5.0) // 5.0 seconds between triggers
{
    paramBinding = AppState::createParameterBinding<Models::DelaySettings>(settings, processor.getAPVTS());
    paramBinding->registerParameters(AppState::createDelayParameters());

    // Initialize active delay state
    activeDelay = {};
}

void Delay::prepareToPlay(double sampleRate, int samplesPerBlock) {
    BaseEffect::prepareToPlay(sampleRate, samplesPerBlock);

    // Maximum delay time of 2 seconds
    const int maxDelayInSamples = static_cast<int>(sampleRate * 2.0);

    // Initialize delay lines
    delayLineLeft = std::make_unique<juce::dsp::DelayLine<float>>(maxDelayInSamples);
    delayLineRight = std::make_unique<juce::dsp::DelayLine<float>>(maxDelayInSamples);

    delayLineLeft->reset();
    delayLineRight->reset();

    // Initialize with specification
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1; // Each delay line handles one channel

    delayLineLeft->prepare(spec);
    delayLineRight->prepare(spec);

    // Set a default delay time
    delayTimeInSamples = static_cast<float>(sampleRate * 0.25f); // 250ms default
    delayLineLeft->setDelay(delayTimeInSamples);
    delayLineRight->setDelay(delayTimeInSamples);

    // Reset active delay state
    activeDelay = {};
}

void Delay::releaseResources() {
    BaseEffect::releaseResources();
    delayLineLeft.reset();
    delayLineRight.reset();
}

void Delay::recalculateDelayTimeInSamples() {
    // Calculate delay time based on rate parameter
    float delayTimeMs;

    if (settings.delayBpmSync && timingManager.getBpm() > 0) {
        // BPM sync mode - calculate from musical note values
        delayTimeMs = calculateDelayTimeFromBPM(settings.delayRate);
    } else {
        // Milliseconds mode - map the 0-100 range to 10-1000ms
        delayTimeMs = juce::jmap(settings.delayRate, 0.0f, 100.0f, 10.0f, 1000.0f);
    }

    // Convert ms to samples
    delayTimeInSamples = (delayTimeMs / 1000.0f) * static_cast<float>(sampleRate);

    // Ensure it doesn't exceed our maximum
    if (delayLineLeft != nullptr) {
        delayTimeInSamples = juce::jmin(delayTimeInSamples,
                                        static_cast<float>(delayLineLeft->getMaximumDelayInSamples()));
    }
}

float Delay::calculateDelayTimeFromBPM(float rate) {
    // Calculate delay time based on BPM
    const double bpm = timingManager.getBpm();
    if (bpm <= 0)
        return 500.0f; // Default to 500ms if BPM is invalid

    // Map rate parameter 0-100 to different note values - use discrete steps
    // 0-10: whole note, 10-30: half note, 30-50: quarter note, 
    // 50-70: eighth note, 70-90: sixteenth note, 90-100: thirty-two second note
    float delayTimeMs;
    const float secondsPerBeat = 60.0f / static_cast<float>(bpm);

    if (rate < 10.0f)
        delayTimeMs = secondsPerBeat * 4.0f * 1000.0f; // Whole note
    else if (rate < 30.0f)
        delayTimeMs = secondsPerBeat * 2.0f * 1000.0f; // Half note
    else if (rate < 50.0f)
        delayTimeMs = secondsPerBeat * 1000.0f; // Quarter note
    else if (rate < 70.0f)
        delayTimeMs = secondsPerBeat * 0.5f * 1000.0f; // Eighth note
    else if (rate < 90.0f)
        delayTimeMs = secondsPerBeat * 0.25f * 1000.0f; // Sixteenth note
    else
        delayTimeMs = secondsPerBeat * 0.125f * 1000.0f; // Thirty-second note

    // Safety check: limit to a reasonable range (10ms to 5 seconds)
    delayTimeMs = juce::jlimit(10.0f, 5000.0f, delayTimeMs);

    return delayTimeMs;
}

bool Delay::shouldApplyDelay() {
    if (activeDelay.isActive) {
        return true;
    }

    // Use base class method for probability check
    return BaseEffect::shouldApplyEffect(settings.delayProbability);
}

void Delay::applyDelayEffect(juce::AudioBuffer<float> &buffer,
                             const std::vector<juce::int64> &triggerSamplePositions) {
    if (!delayLineLeft || !delayLineRight || buffer.getNumChannels() == 0 || settings.delayMix <= 0.0f)
        return;

    recalculateDelayTimeInSamples();

    // Apply delay effect
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Check if we should apply the effect
    if (!shouldApplyDelay() && !activeDelay.isActive)
        return;

    // Create temporary buffer for dry signal
    juce::AudioBuffer<float> delayBuffer;
    delayBuffer.makeCopyOf(buffer);

    // Ensure delay time is set and within limits
    float safeDelayTime = juce::jlimit(1.0f, static_cast<float>(delayLineLeft->getMaximumDelayInSamples() - 1),
                                       delayTimeInSamples);
    delayLineLeft->setDelay(safeDelayTime);
    delayLineRight->setDelay(safeDelayTime);

    // Calculate feedback amount
    float feedbackAmount = settings.delayFeedback;

    // Process the delay through the delay lines
    if (settings.delayPingPong && numChannels > 1) {
        for (int sample = 0; sample < numSamples; ++sample) {
            processPingPongDelay(buffer, delayBuffer, feedbackAmount, sample);
        }
    } else {
        for (int sample = 0; sample < numSamples; ++sample) {
            for (int channel = 0; channel < numChannels; ++channel) {
                processStereoDelay(buffer, delayBuffer, feedbackAmount, channel, sample);
            }
        }
    }

    // Now mix the dry and wet signals based on probability
    float wetMix = settings.delayMix;

    if (settings.delayProbability >= 99.9f) {
        // Apply to entire buffer
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            mixWetDrySignals(buffer.getWritePointer(channel),
                             delayBuffer.getReadPointer(channel),
                             wetMix,
                             buffer.getNumSamples());
        }
    } else if (!triggerSamplePositions.empty() || activeDelay.isActive) {
        // Handle ongoing delay
        if (activeDelay.isActive) {
            processActiveDelay(buffer, delayBuffer, wetMix);
        }
            // Handle new trigger position
        else if (!triggerSamplePositions.empty() && isEffectEnabledForSample(Models::EffectType::DELAY) && hasMinTimePassed()) {
            processNewDelayTrigger(buffer, delayBuffer, triggerSamplePositions, wetMix);
        }
    }
}

void Delay::processActiveDelay(juce::AudioBuffer<float> &buffer,
                               const juce::AudioBuffer<float> &delayBuffer,
                               float wetMix) {
    // Calculate how much of the delay is left in this buffer
    juce::int64 remainingDuration = activeDelay.duration - activeDelay.currentPosition;

    if (remainingDuration > 0) {
        // Apply delay to the remaining duration
        int endSample = juce::jmin(buffer.getNumSamples(), static_cast<int>(remainingDuration));

        // Apply delay with proper gain preservation and fade-out
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            float *dry = buffer.getWritePointer(channel);
            const float *wet = delayBuffer.getReadPointer(channel);

            for (int sample = 0; sample < endSample; ++sample) {
                // Calculate fade-out envelope
                float progress = (activeDelay.currentPosition + sample) / static_cast<float>(activeDelay.duration);
                float fadeOut = 1.0f;
                applyFadeOut(fadeOut, progress);

                // Mix signals with fade
                float dryGain = std::max(std::cos(wetMix * juce::MathConstants<float>::halfPi), 0.0f);
                float wetGain = std::sin(wetMix * juce::MathConstants<float>::halfPi) * fadeOut;
                dry[sample] = dry[sample] * dryGain + wet[sample] * wetGain;
            }
        }

        // Update position
        activeDelay.currentPosition += endSample;

        // Check if delay is complete
        if (activeDelay.currentPosition >= activeDelay.duration) {
            activeDelay.isActive = false;
        }
    }
}

void Delay::processNewDelayTrigger(juce::AudioBuffer<float> &buffer,
                                   const juce::AudioBuffer<float> &delayBuffer,
                                   const std::vector<juce::int64> &triggerSamplePositions,
                                   float wetMix) {
    int startSample = triggerSamplePositions[0];
    if (startSample >= 0 && startSample < buffer.getNumSamples()) {
        // Update the last trigger time
        lastTriggerSample = timingManager.getSamplePosition() + startSample;

//      3 seconds duration
        auto noteDuration = static_cast<juce::int64>(sampleRate * 3.0);

        // Start a new delay effect
        activeDelay = {
                startSample,
                noteDuration,
                0,
                true
        };

        // Apply delay to the note region
        int endSample = juce::jmin(buffer.getNumSamples(),
                                   startSample + static_cast<int>(noteDuration));

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            float *dry = buffer.getWritePointer(channel);
            const float *wet = delayBuffer.getReadPointer(channel);

            for (int sample = startSample; sample < endSample; ++sample) {
                float dryGain = std::max(std::cos(wetMix * juce::MathConstants<float>::halfPi), 0.0f);
                float wetGain = std::sin(wetMix * juce::MathConstants<float>::halfPi);
                dry[sample] = dry[sample] * dryGain + wet[sample] * wetGain;
            }
        }

        // Update position
        activeDelay.currentPosition = endSample - startSample;
    }
}

void Delay::processPingPongDelay(const juce::AudioBuffer<float> &buffer, juce::AudioBuffer<float> &delayBuffer,
                                 float feedbackAmount, int sample) {
    // Get current samples for processing
    // Mix left and right inputs to mono
    float monoInput = (buffer.getSample(0, sample) + buffer.getSample(1, sample)) * 0.5f;

    // Get delayed outputs
    float leftDelayed = delayLineLeft->popSample(0);
    float rightDelayed = delayLineRight->popSample(0);

    // Push mono input to left delay line only, with feedback from right
    delayLineLeft->pushSample(0, monoInput + rightDelayed * feedbackAmount);

    // Push only feedback from left to right delay line (no direct input)
    delayLineRight->pushSample(0, leftDelayed * feedbackAmount);

    // Store delayed samples in the delay buffer
    delayBuffer.setSample(0, sample, leftDelayed);
    delayBuffer.setSample(1, sample, rightDelayed);
}

void Delay::processStereoDelay(const juce::AudioBuffer<float> &buffer,
                               juce::AudioBuffer<float> &delayBuffer,
                               float feedbackAmount,
                               int channel,
                               int sample) {
    // Get current sample for processing
    float inputSample = buffer.getSample(channel, sample);

    // Get the appropriate delay line
    auto &delayLine = (channel == 0) ? *delayLineLeft : *delayLineRight;

    // Get delayed sample from the delay line
    float delayedSample = delayLine.popSample(0);

    // Push the input sample WITH feedback to the delay line
    delayLine.pushSample(0, inputSample + delayedSample * feedbackAmount);

    // Store only the delayed sample in the output buffer
    delayBuffer.setSample(channel, sample, delayedSample);
}