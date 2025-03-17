//
// Created by Shaked Melman on 05/03/2025.
//

#include "Stutter.h"
#include "../Sampler/Sampler.h"

Stutter::Stutter(PluginProcessor &p)
        : BaseEffect(p, 3.0) {
    paramBinding = AppState::createParameterBinding<Models::StutterSettings>(settings, processor.getAPVTS());
    paramBinding->registerParameters(AppState::createStutterParameters());

    // Initialize stutter state
    isStuttering = false;
    stutterPosition = 0;
    stutterLength = 0;
    stutterRepeatCount = 0;
    stutterRepeatsTotal = 2;
}

void Stutter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    BaseEffect::prepareToPlay(sampleRate, samplesPerBlock);

    int maxStutterSamples = static_cast<int>(sampleRate * 5.0); // 5 seconds of audio
    stutterBuffer.setSize(2, maxStutterSamples);
    stutterBuffer.clear();

    // History buffer should be large enough to capture several beats
    historyBufferSize = static_cast<int>(sampleRate * 5.0); // 5 seconds of history
    historyBuffer.setSize(2, historyBufferSize);
    historyBuffer.clear();
    historyWritePosition = 0;

    // Reset stutter state
    resetStutterState();
}

void Stutter::releaseResources() {
    BaseEffect::releaseResources();
    stutterBuffer.setSize(0, 0);
    historyBuffer.setSize(0, 0);
}

void Stutter::applyStutterEffect(juce::AudioBuffer<float> &buffer,
                                 std::vector<juce::int64> triggerSamplePositions) {
    // Store buffer info for later use
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    addToHistory(buffer);
    handleTransportLoopDetection();

//     Process stutter effect
    if (isStuttering) {
        processActiveStutter(buffer, numSamples, numChannels);
    } else if (shouldStutter() && !triggerSamplePositions.empty() && hasMinTimePassed() &&
               isEffectEnabledForSample(Models::EffectType::STUTTER)) {
        startStutterAtPosition(
                buffer, triggerSamplePositions[0], numSamples, numChannels);
    }
}

bool Stutter::shouldStutter() {
    // Use the base class method with stutter probability
    return BaseEffect::shouldApplyEffect(settings.stutterProbability);
}

void Stutter::processActiveStutter(juce::AudioBuffer<float> &buffer,
                                   int numSamples,
                                   int numChannels) {
    // Safety check for division by zero
    if (stutterLength <= 0) {
        resetStutterState();
        return;
    }

    // Safety check for stutter buffer size
    if (stutterBuffer.getNumSamples() < stutterLength) {
        resetStutterState();
        return;
    }

    // Create a temporary buffer for mixing
    juce::AudioBuffer<float> tempBuffer(numChannels, numSamples);
    tempBuffer.clear();

    // Copy stutter data to temp buffer with looping
    copyStutterDataToBuffer(tempBuffer, numSamples, numChannels);

    // Apply crossfade between original and stutter buffer
    applyStutterCrossfade(buffer, tempBuffer, numSamples, numChannels);

    // Update stutter position and repeat count
    updateStutterPosition(numSamples);

    // If we've just marked the stutter to end (via endStutterEffect),
    // but haven't yet fully reset the state, do a complete reset now
    if (!isStuttering && stutterRepeatCount >= stutterRepeatsTotal) {
        resetStutterState();
    }
}

void Stutter::copyStutterDataToBuffer(juce::AudioBuffer<float> &tempBuffer,
                                      int numSamples,
                                      int numChannels) {
    // Safety check for stutter buffer size and stutter length
    if (stutterLength <= 0 || stutterBuffer.getNumSamples() < stutterLength) {
        return; // Don't copy anything
    }

    for (int channel = 0;
         channel < juce::jmin(numChannels, stutterBuffer.getNumChannels());
         ++channel) {
        float *channelData = tempBuffer.getWritePointer(channel);
        const float *stutterData = stutterBuffer.getReadPointer(channel);


        for (int i = 0; i < numSamples; ++i) {
            // Loop through the stutter buffer with bounds checking
            int stutterIndex = (stutterPosition + i) % stutterLength;

            // Additional safety check
            if (stutterIndex < 0 || stutterIndex >= stutterBuffer.getNumSamples()) {
                channelData[i] = 0.0f; // Safety - use silence
            } else {
                channelData[i] = stutterData[stutterIndex];
            }
        }
    }
}

void Stutter::applyStutterCrossfade(juce::AudioBuffer<float> &buffer,
                                    const juce::AudioBuffer<float> &tempBuffer,
                                    int numSamples,
                                    int numChannels) {
    float fadeLength = juce::jmin(100, numSamples); // 100 samples for crossfade

    for (int channel = 0; channel < numChannels; ++channel) {
        float *mainData = buffer.getWritePointer(channel);
        const float *stutterData = tempBuffer.getReadPointer(channel);

        // Apply crossfade at the start of stutter (only during first cycle)
        if (stutterPosition < fadeLength && stutterRepeatCount == 0) {
            for (int i = 0; i < fadeLength; ++i) {
                float alpha = static_cast<float>(i) / fadeLength;
                mainData[i] = mainData[i] * (1.0f - alpha) + stutterData[i] * alpha;
            }

            // Copy the rest of the stutter data after crossfade
            if (fadeLength < numSamples) {
                buffer.copyFrom(channel, fadeLength, tempBuffer, channel, fadeLength, numSamples - fadeLength);
            }
        }
            // Apply crossfade at the end of stutter effect (when near the end of the last repeat)
        else if (stutterRepeatCount == stutterRepeatsTotal - 1 &&
                 stutterPosition > stutterLength - numSamples - fadeLength &&
                 stutterPosition <= stutterLength - fadeLength) {
            // We're approaching the end of the last repeat, apply fadeout

            // Calculate how many samples are left in the stutter
            int samplesRemaining = stutterLength - stutterPosition;

            // Apply crossfade
            for (int i = 0; i < samplesRemaining; ++i) {
                float alpha = 1.0f - (static_cast<float>(i) / samplesRemaining);
                mainData[i] = mainData[i] * (1.0f - alpha) + stutterData[i] * alpha;
            }

            // For the rest of the buffer after the stutter ends, just use the original buffer
            // (which is already in mainData)
        } else {
            // Copy all the stutter data in other cases
            buffer.copyFrom(channel, 0, tempBuffer, channel, 0, numSamples);
        }
    }
}

void Stutter::updateStutterPosition(int numSamples) {
    // Safety check for division by zero
    if (stutterLength <= 0) {
        resetStutterState();
        return;
    }

    int oldPosition = stutterPosition;
    stutterPosition = (stutterPosition + numSamples) % stutterLength;

    // Log position wraparound
    if (stutterPosition < oldPosition) {
        stutterRepeatCount++;
        if (stutterRepeatCount >= stutterRepeatsTotal) {
            endStutterEffect();
        }
    }

    // Safety timeout - prevent stutters from lasting too long
    if (stutterRepeatCount > 8) // Maximum number of repeats to prevent hanging
    {
        resetStutterState();
    }
}

void Stutter::endStutterEffect() {
    // We will apply the crossfade when actually ending the stutter in processActiveStutter
    // This just marks that we should end the stutter
    isStuttering = false;

    // Don't reset position and length immediately so the final crossfade can be applied
    // in the next processActiveStutter call

    // Mark that we've completed all repeats to trigger final crossfade
    stutterRepeatCount = stutterRepeatsTotal;
    lastTriggerSample = timingManager.getSamplePosition();
}

void Stutter::resetStutterState() {
    // Fully reset all state variables
    isStuttering = false;
    stutterPosition = 0;
    stutterLength = 0;
    stutterRepeatCount = 0;
    stutterRepeatsTotal = 0;
}

void Stutter::startStutterAtPosition(juce::AudioBuffer<float> &buffer,
                                     juce::int64 samplePosition,
                                     int numSamples,
                                     int numChannels) {
    // Choose a rate (1/8, 1/16 note, etc.)
    Models::RateOption selectedRate = selectRandomRate();

    // Calculate stutter length based on musical timing
    int captureLength =
            static_cast<int>(timingManager.getNoteDurationInSamples(selectedRate));

    // Limit to a reasonable value (and log any adjustment)
    int originalLength = captureLength;
    captureLength = juce::jmin(captureLength, stutterBuffer.getNumSamples());
    // Capture the musical segment starting from the note position
    captureFromHistory(samplePosition, captureLength);

    // Configure stutter parameters
    isStuttering = true;
    stutterLength = captureLength;
    stutterPosition = 0;
    stutterRepeatsTotal = 2 + random.nextInt(3); // 2-4 repeats
    stutterRepeatCount = 0;

    // Apply stutter effect immediately for the rest of this buffer
    applyImmediateStutterEffect(buffer, samplePosition, numSamples, numChannels);
}

void Stutter::applyImmediateStutterEffect(juce::AudioBuffer<float> &buffer,
                                          juce::int64 samplePosition,
                                          int numSamples,
                                          int numChannels) {
    juce::AudioBuffer<float> tempBuffer(numChannels, numSamples - samplePosition);
    tempBuffer.clear();

    for (int channel = 0;
         channel < juce::jmin(numChannels, stutterBuffer.getNumChannels());
         ++channel) {
        float *channelData = tempBuffer.getWritePointer(channel);
        const float *stutterData = stutterBuffer.getReadPointer(channel);

        for (int i = 0; i < numSamples - samplePosition; ++i) {
            int stutterIndex = i % stutterLength;
            channelData[i] = stutterData[stutterIndex];
        }

        // Copy the stutter data back to the main buffer starting at the note position
        buffer.copyFrom(
                channel, samplePosition, tempBuffer, channel, 0, numSamples - samplePosition);
    }

    // Update stutter position for the next buffer
    stutterPosition = (numSamples - samplePosition) % stutterLength;
}

void Stutter::addToHistory(const juce::AudioBuffer<float> &buffer) {
    // Add the current buffer to the history circular buffer
    int numSamples = buffer.getNumSamples();

    for (int channel = 0;
         channel < juce::jmin(buffer.getNumChannels(), historyBuffer.getNumChannels());
         ++channel) {
        if (historyWritePosition + numSamples <= historyBufferSize) {
            // Simple copy if it doesn't wrap around
            historyBuffer.copyFrom(
                    channel, historyWritePosition, buffer, channel, 0, numSamples);
        } else {
            // Handle wrap-around case
            int firstPartSize = historyBufferSize - historyWritePosition;
            historyBuffer.copyFrom(
                    channel, historyWritePosition, buffer, channel, 0, firstPartSize);
            historyBuffer.copyFrom(
                    channel, 0, buffer, channel, firstPartSize, numSamples - firstPartSize);
        }
    }

    // Update write position with wrap-around
    historyWritePosition = (historyWritePosition + numSamples) % historyBufferSize;
}

void Stutter::captureFromHistory(juce::int64 triggerSamplePosition, int lengthToCapture) {
    // Determine starting position in history buffer
    // We need to account for the fact that:
    // 1. historyWritePosition points to where the *next* buffer will be written
    // 2. triggerSamplePosition is relative to the *current* buffer

    // Calculate samples from end of current buffer
    juce::int64 samplesFromEnd = currentBufferSize - triggerSamplePosition;

    // Calculate absolute history buffer position where trigger point is
    int historyTriggerPos =
            (historyWritePosition - samplesFromEnd + historyBufferSize) % historyBufferSize;

    // Ensure stutter buffer is big enough
    if (stutterBuffer.getNumSamples() < lengthToCapture) {
        // Log the resize operation for debugging
        stutterBuffer.setSize(
                stutterBuffer.getNumChannels(), lengthToCapture, true, true, true);
    }

    // Clear stutter buffer
    stutterBuffer.clear();

    // Copy the segment from history buffer to stutter buffer
    for (int channel = 0; channel < juce::jmin(stutterBuffer.getNumChannels(),
                                               historyBuffer.getNumChannels());
         ++channel) {
        if (historyTriggerPos + lengthToCapture <= historyBufferSize) {
            // Simple case - no wrap-around
            stutterBuffer.copyFrom(
                    channel, 0, historyBuffer, channel, historyTriggerPos, lengthToCapture);
        } else {
            // Handle wrap-around case
            int firstPartSize = historyBufferSize - historyTriggerPos;

            // Copy first part (from trigger position to end of history buffer)
            stutterBuffer.copyFrom(
                    channel, 0, historyBuffer, channel, historyTriggerPos, firstPartSize);

            // Copy second part (from start of history buffer)
            int secondPartSize = lengthToCapture - firstPartSize;
            stutterBuffer.copyFrom(channel,
                                   firstPartSize,
                                   historyBuffer,
                                   channel,
                                   0,
                                   secondPartSize);

        }

        // Verify the data is not silent (for debugging)
        float sum = 0.0f;
        for (int i = 0; i < lengthToCapture; ++i) {
            sum += std::abs(stutterBuffer.getSample(channel, i));
        }
    }
}

void Stutter::handleTransportLoopDetection() {
    // Check if we've detected a loop in the transport (from TimingManager)
    if (timingManager.wasLoopDetected()) {
        // Reset stuttering state when transport loops
        isStuttering = false;
        stutterPosition = 0;
        stutterLength = 0;
        stutterRepeatCount = 0;
        stutterRepeatsTotal = 0;

        // Clear the loop detection flag
        timingManager.clearLoopDetection();
    }
}

Models::RateOption Stutter::selectRandomRate() {
    Models::RateOption rates[] = {Models::RATE_1_8,
                                  Models::RATE_1_16,
                                  Models::RATE_1_32};

    float randomValue = random.nextFloat();

    if (randomValue < 0.33f) {
        return rates[0]; // 1/8 note
    } else if (randomValue < 0.75f) {
        return rates[1]; // 1/16 note
    } else {
        return rates[2]; // 1/32 note
    }
}