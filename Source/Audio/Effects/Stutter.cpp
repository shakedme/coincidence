//
// Created by Shaked Melman on 05/03/2025.
//

#include "Stutter.h"

Stutter::Stutter()
        : BaseEffect() {
    // Initialize stutter state
    isStuttering = false;
    stutterPosition = 0;
    stutterLength = 0;
    stutterRepeatCount = 0;
    stutterRepeatsTotal = 2;
}

void Stutter::initialize(PluginProcessor &p) {
    BaseEffect::initialize(p);

    paramBinding = AppState::createParameterBinding<Models::StutterSettings>(settings, p.getAPVTS());
    paramBinding->registerParameters(AppState::createStutterParameters());
}

void Stutter::prepare(const juce::dsp::ProcessSpec &spec) {
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

void Stutter::reset() {
    stutterBuffer.setSize(0, 0);
    historyBuffer.setSize(0, 0);
}

void Stutter::process(const juce::dsp::ProcessContextReplacing<float> &context) {
    auto &&inBlock = context.getInputBlock();
    auto &&outBlock = context.getOutputBlock();
    auto numSamples = inBlock.getNumSamples();
    auto numChannels = inBlock.getNumChannels();

    jassert(inBlock.getNumChannels() == outBlock.getNumChannels());
    jassert(inBlock.getNumSamples() == outBlock.getNumSamples());

    // Update the current buffer size for consistent behavior
    currentBufferSize = static_cast<int>(numSamples);

    // Add the current input to history buffer
    addToHistoryFromBlock(inBlock);

    // Handle transport loop detection
    handleTransportLoopDetection();

    // Check for MIDI triggers
    std::vector<juce::int64> triggerSamplePositions = checkForMidiTriggers(midiMessages);

    // Process stutter effect directly on the blocks
    if (isStuttering) {
        processActiveStutterBlock(outBlock, numSamples, numChannels);
    } else if (shouldStutter() && !triggerSamplePositions.empty() && hasMinTimePassed()) {
        // If we need to start stuttering, first copy input to output
        outBlock.copyFrom(inBlock);

        // Then apply stutter starting at the trigger position
        startStutterAtPositionBlock(outBlock, triggerSamplePositions[0], numSamples, numChannels);
    } else {
        // If not stuttering, simply copy input to output
        outBlock.copyFrom(inBlock);
    }
}

void Stutter::addToHistoryFromBlock(const juce::dsp::AudioBlock<const float> &block) {
    // Add the current block to the history circular buffer
    int numSamples = block.getNumSamples();

    for (size_t channel = 0; channel < std::min(block.getNumChannels(),
                                                static_cast<size_t>(historyBuffer.getNumChannels())); ++channel) {
        const float *blockData = block.getChannelPointer(channel);

        if (historyWritePosition + numSamples <= historyBufferSize) {
            // Simple copy if it doesn't wrap around
            historyBuffer.copyFrom(channel, historyWritePosition, blockData, numSamples);
        } else {
            // Handle wrap-around case
            int firstPartSize = historyBufferSize - historyWritePosition;
            historyBuffer.copyFrom(channel, historyWritePosition, blockData, firstPartSize);
            historyBuffer.copyFrom(channel, 0, blockData + firstPartSize, numSamples - firstPartSize);
        }
    }

    // Update write position with wrap-around
    historyWritePosition = (historyWritePosition + numSamples) % historyBufferSize;
}

void Stutter::processActiveStutterBlock(juce::dsp::AudioBlock<float> &outBlock,
                                        int numSamples,
                                        int numChannels) {
    // Safety checks for stutter state
    if (stutterLength <= 0 || stutterBuffer.getNumSamples() < stutterLength) {
        resetStutterState();
        return;
    }

    // Copy stutter data to output block with looping
    for (size_t channel = 0; channel < std::min(numChannels, stutterBuffer.getNumChannels()); ++channel) {
        float *outData = outBlock.getChannelPointer(channel);
        const float *stutterData = stutterBuffer.getReadPointer(channel);

        // Handle crossfade at the start of stutter (only during first cycle)
        float fadeLength = std::min(100, numSamples);

        if (stutterPosition < fadeLength && stutterRepeatCount == 0) {
            // Apply crossfade at the beginning
            for (int i = 0; i < fadeLength; ++i) {
                float alpha = static_cast<float>(i) / fadeLength;
                // Keep original data with (1-alpha) weight and add stutter data with alpha weight
                int stutterIndex = (stutterPosition + i) % stutterLength;
                outData[i] = outData[i] * (1.0f - alpha) + stutterData[stutterIndex] * alpha;
            }

            // Fill the rest without crossfade
            for (int i = fadeLength; i < numSamples; ++i) {
                int stutterIndex = (stutterPosition + i) % stutterLength;
                outData[i] = stutterData[stutterIndex];
            }
        }
            // Apply crossfade at the end of stutter effect (when near the end of the last repeat)
        else if (stutterRepeatCount == stutterRepeatsTotal - 1 &&
                 stutterPosition > stutterLength - numSamples - fadeLength &&
                 stutterPosition <= stutterLength - fadeLength) {
            // Calculate how many samples are left in the stutter
            int samplesRemaining = stutterLength - stutterPosition;

            // Apply crossfade for the remaining samples
            for (int i = 0; i < samplesRemaining; ++i) {
                float alpha = 1.0f - (static_cast<float>(i) / samplesRemaining);
                int stutterIndex = (stutterPosition + i) % stutterLength;
                outData[i] = outData[i] * (1.0f - alpha) + stutterData[stutterIndex] * alpha;
            }

            // The rest of the buffer is just passed through (already in outData)
        } else {
            // Regular case - just copy stutter data
            for (int i = 0; i < numSamples; ++i) {
                int stutterIndex = (stutterPosition + i) % stutterLength;
                outData[i] = stutterData[stutterIndex];
            }
        }
    }

    // Update stutter position
    updateStutterPosition(numSamples);

    // Check if we need to reset stutter state
    if (!isStuttering && stutterRepeatCount >= stutterRepeatsTotal) {
        resetStutterState();
    }
}

void Stutter::startStutterAtPositionBlock(juce::dsp::AudioBlock<float> &outBlock,
                                          juce::int64 samplePosition,
                                          int numSamples,
                                          int numChannels) {
    // Choose a rate (1/8, 1/16 note, etc.)
    Models::RateOption selectedRate = selectRandomRate();

    // Calculate stutter length based on musical timing
    int captureLength = static_cast<int>(timingManagerPtr->getNoteDurationInSamples(selectedRate));

    // Limit to a reasonable value
    captureLength = juce::jmin(captureLength, stutterBuffer.getNumSamples());

    // Capture the musical segment starting from the note position
    captureFromHistory(samplePosition, captureLength);

    // Configure stutter parameters
    isStuttering = true;
    stutterLength = captureLength;
    stutterPosition = 0;
    stutterRepeatsTotal = 2 + random.nextInt(3); // 2-4 repeats
    stutterRepeatCount = 0;

    // Apply stutter effect immediately for the rest of this block
    // Apply from samplePosition to the end of the block
    for (size_t channel = 0; channel < std::min(numChannels, stutterBuffer.getNumChannels()); ++channel) {
        float *outData = outBlock.getChannelPointer(channel);
        const float *stutterData = stutterBuffer.getReadPointer(channel);

        // Replace data from trigger position to end of block with stutter data
        for (int i = samplePosition; i < numSamples; ++i) {
            int stutterIndex = (i - samplePosition) % stutterLength;
            outData[i] = stutterData[stutterIndex];
        }
    }

    // Update stutter position for the next buffer
    stutterPosition = (numSamples - samplePosition) % stutterLength;
}

bool Stutter::shouldStutter() {
    return BaseEffect::shouldApplyEffect(settings.stutterProbability);
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
    lastTriggerSample = timingManagerPtr->getSamplePosition();
}

void Stutter::resetStutterState() {
    // Fully reset all state variables
    isStuttering = false;
    stutterPosition = 0;
    stutterLength = 0;
    stutterRepeatCount = 0;
    stutterRepeatsTotal = 0;
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
    if (timingManagerPtr->wasLoopDetected()) {
        // Reset stuttering state when transport loops
        isStuttering = false;
        stutterPosition = 0;
        stutterLength = 0;
        stutterRepeatCount = 0;
        stutterRepeatsTotal = 0;

        // Clear the loop detection flag
        timingManagerPtr->clearLoopDetection();
    }
}

Models::RateOption Stutter::selectRandomRate() {
    Models::RateOption rates[] = {Models::RATE_1_8,
                                  Models::RATE_1_16,
                                  Models::RATE_1_32};

    float randomValue = random.nextFloat();

    if (randomValue < 0.4f) {
        return rates[0]; // 1/8 note
    } else if (randomValue < 0.8f) {
        return rates[1]; // 1/16 note
    } else {
        return rates[2]; // 1/32 note
    }
}

std::vector<juce::int64> Stutter::checkForMidiTriggers(const juce::MidiBuffer &midiMessages) {
    std::vector<juce::int64> triggerPositions;

    // Look for MIDI note-on events to use as reference points
    if (!midiMessages.isEmpty()) {
        for (const auto metadata: midiMessages) {
            auto message = metadata.getMessage();
            if (message.isNoteOn()) {
                // Found a note-on - this is a good point to start effects
                triggerPositions.push_back(metadata.samplePosition);
            }
        }
    }

    return triggerPositions;
}