// GlitchEngine.h modifications

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "TimingManager.h"
#include "GlitchEngine.h"

GlitchEngine::GlitchEngine(std::shared_ptr<TimingManager> t)
    : timingManager(t)
{
    // Initialize random generator
    random.setSeedRandomly();
}

GlitchEngine::~GlitchEngine()
{
    releaseResources();
}

void GlitchEngine::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Store audio settings
    this->sampleRate = sampleRate;
    this->bufferSize = samplesPerBlock;

    // Initialize stutter buffer with reasonable size (4 bars at 120BPM)
    int maxStutterSamples = static_cast<int>(sampleRate * 8.0);
    stutterBuffer.setSize(2, maxStutterSamples);
    stutterBuffer.clear();

    // Initialize history buffer to store 2 seconds of audio
    historyBufferSize = static_cast<int>(sampleRate * 2.0);
    historyBuffer.setSize(2, historyBufferSize);
    historyBuffer.clear();
    historyWritePosition = 0;

    // Reset processing state
    isStuttering = false;
    stutterPosition = 0;
    stutterLength = 0;
    stutterRepeatCount = 0;
}

void GlitchEngine::releaseResources()
{
    // Reset state
    isStuttering = false;

    // Clear buffers
    stutterBuffer.clear();
    historyBuffer.clear();
}

void GlitchEngine::processAudio(juce::AudioBuffer<float>& buffer,
                                juce::AudioPlayHead* playHead,
                                const juce::MidiBuffer& midiMessages)
{
    // Get timing info from playhead
    juce::Optional<juce::AudioPlayHead::PositionInfo> posInfo;

    if (playHead != nullptr)
    {
        timingManager->updateTimingInfo(playHead);
        posInfo = playHead->getPosition();
    }

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    currentBufferSize = numSamples;

    // Add current buffer to history first
    addToHistory(buffer);

    // Safety check - if probability is 0, ensure we're not stuttering
    if (stutterProbability <= 0.0f)
    {
        isStuttering = false;
    }

    // Check if we've detected a loop in the transport (from TimingManager)
    if (timingManager->wasLoopDetected())
    {
        // Reset stuttering state when transport loops
        isStuttering = false;
        stutterPosition = 0;
        stutterLength = 0;
        stutterRepeatCount = 0;
        stutterRepeatsTotal = 0;

        // Clear the loop detection flag
        timingManager->clearLoopDetection();
    }

    // If we're already stuttering, continue the stutter effect
    if (isStuttering)
    {
        // Create a temporary buffer for mixing
        juce::AudioBuffer<float> tempBuffer(numChannels, numSamples);
        tempBuffer.clear();

        // Safety check for division by zero
        if (stutterLength <= 0)
        {
            isStuttering = false;
            stutterPosition = 0;
            stutterLength = 0;
            stutterRepeatCount = 0;
            return;
        }

        for (int channel = 0;
             channel < juce::jmin(numChannels, stutterBuffer.getNumChannels());
             ++channel)
        {
            float* channelData = tempBuffer.getWritePointer(channel);
            const float* stutterData = stutterBuffer.getReadPointer(channel);

            for (int i = 0; i < numSamples; ++i)
            {
                // Loop through the stutter buffer
                int stutterIndex = (stutterPosition + i) % stutterLength;
                channelData[i] = stutterData[stutterIndex];
            }
        }

        // Crossfade between original and stutter buffer to avoid clicks
        float fadeLength = juce::jmin(100, numSamples); // 100 samples for crossfade
        for (int channel = 0; channel < numChannels; ++channel)
        {
            float* mainData = buffer.getWritePointer(channel);
            const float* stutterData = tempBuffer.getReadPointer(channel);

            // Apply crossfade at the start of stutter
            if (stutterPosition < fadeLength)
            {
                for (int i = 0; i < fadeLength; ++i)
                {
                    float alpha = static_cast<float>(i) / fadeLength;
                    mainData[i] = mainData[i] * (1.0f - alpha) + stutterData[i] * alpha;
                }
            }
            else
            {
                // Copy the rest of the stutter data
                buffer.copyFrom(channel, 0, tempBuffer, channel, 0, numSamples);
            }
        }

        // Update stutter position
        int oldPosition = stutterPosition;
        stutterPosition = (stutterPosition + numSamples) % stutterLength;

        // Log position wraparound
        if (stutterPosition < oldPosition) {
            // We've completed a full loop through the stutter buffer
            stutterRepeatCount++;
            if (stutterRepeatCount >= stutterRepeatsTotal)
            {
                // End the stutter with a crossfade
                for (int channel = 0; channel < numChannels; ++channel)
                {
                    float* mainData = buffer.getWritePointer(channel);
                    const float* stutterData = tempBuffer.getReadPointer(channel);

                    for (int i = 0; i < fadeLength; ++i)
                    {
                        float alpha = 1.0f - (static_cast<float>(i) / fadeLength);
                        mainData[i] =
                            mainData[i] * (1.0f - alpha) + stutterData[i] * alpha;
                    }
                }

                // Fully reset the stutter state
                isStuttering = false;
                stutterPosition = 0;
                stutterLength = 0;
                stutterRepeatCount = 0;
                stutterRepeatsTotal = 0;
            }
        }

        // Safety timeout - prevent stutters from lasting too long
        if (stutterRepeatCount > 8) // Maximum number of repeats to prevent hanging
        {
            isStuttering = false;
            stutterPosition = 0;
            stutterLength = 0;
            stutterRepeatCount = 0;
            stutterRepeatsTotal = 0;
        }
    }
    // If not currently stuttering, check if we should start
    else if (stutterProbability > 0.0f)
    {
        // Apply probability check first
        if (random.nextFloat() < (stutterProbability / 100.0f))
        {
            // Look for MIDI note-on events to use as stutter points
            if (!midiMessages.isEmpty())
            {
                for (const auto metadata : midiMessages)
                {
                    auto message = metadata.getMessage();
                    if (message.isNoteOn())
                    {
                        // Found a note-on - this is a good point to start stuttering
                        int samplePosition = metadata.samplePosition;
                        // Choose a rate (1/8, 1/16 note, etc.)
                        Params::RateOption selectedRate = selectRandomRate();

                        // Calculate stutter length based on musical timing
                        Params::GeneratorSettings settings;
                        int captureLength = static_cast<int>(
                            timingManager->getNoteDurationInSamples(selectedRate, settings));

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

                        // Apply stutter effect immediately for the rest of this buffer
                        juce::AudioBuffer<float> tempBuffer(numChannels, numSamples - samplePosition);
                        tempBuffer.clear();

                        for (int channel = 0; channel < juce::jmin(numChannels, stutterBuffer.getNumChannels()); ++channel)
                        {
                            float* channelData = tempBuffer.getWritePointer(channel);
                            const float* stutterData = stutterBuffer.getReadPointer(channel);

                            for (int i = 0; i < numSamples - samplePosition; ++i)
                            {
                                int stutterIndex = i % stutterLength;
                                channelData[i] = stutterData[stutterIndex];
                            }

                            // Copy the stutter data back to the main buffer starting at the note position
                            buffer.copyFrom(channel, samplePosition, tempBuffer, channel, 0, numSamples - samplePosition);
                        }

                        // Update stutter position for the next buffer
                        stutterPosition = (numSamples - samplePosition) % stutterLength;

                        break; // Only use the first note-on event
                    }
                }
            }
        }
    }
    // If not stuttering, the original audio in buffer passes through unchanged
}

void GlitchEngine::addToHistory(const juce::AudioBuffer<float>& buffer)
{
    // Add the current buffer to the history circular buffer
    int numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < juce::jmin(buffer.getNumChannels(), historyBuffer.getNumChannels()); ++channel)
    {
        if (historyWritePosition + numSamples <= historyBufferSize)
        {
            // Simple copy if it doesn't wrap around
            historyBuffer.copyFrom(channel, historyWritePosition, buffer, channel, 0, numSamples);
        }
        else
        {
            // Handle wrap-around case
            int firstPartSize = historyBufferSize - historyWritePosition;
            historyBuffer.copyFrom(channel, historyWritePosition, buffer, channel, 0, firstPartSize);
            historyBuffer.copyFrom(channel, 0, buffer, channel, firstPartSize, numSamples - firstPartSize);
        }
    }

    // Update write position with wrap-around
    historyWritePosition = (historyWritePosition + numSamples) % historyBufferSize;
}

void GlitchEngine::captureFromHistory(int triggerSamplePosition, int lengthToCapture)
{
    // Determine starting position in history buffer
    // We need to account for the fact that:
    // 1. historyWritePosition points to where the *next* buffer will be written
    // 2. triggerSamplePosition is relative to the *current* buffer

    // Calculate samples from end of current buffer
    int samplesFromEnd = currentBufferSize - triggerSamplePosition;

    // Calculate absolute history buffer position where trigger point is
    int historyTriggerPos = (historyWritePosition - samplesFromEnd + historyBufferSize) % historyBufferSize;

    // Ensure stutter buffer is big enough
    if (stutterBuffer.getNumSamples() < lengthToCapture) {
        stutterBuffer.setSize(stutterBuffer.getNumChannels(), lengthToCapture, true, true, true);
    }

    // Clear stutter buffer
    stutterBuffer.clear();

    // Copy the segment from history buffer to stutter buffer
    for (int channel = 0; channel < juce::jmin(stutterBuffer.getNumChannels(), historyBuffer.getNumChannels()); ++channel)
    {
        if (historyTriggerPos + lengthToCapture <= historyBufferSize)
        {
            // Simple case - no wrap-around
            stutterBuffer.copyFrom(channel, 0, historyBuffer, channel, historyTriggerPos, lengthToCapture);
        }
        else
        {
            // Handle wrap-around case
            int firstPartSize = historyBufferSize - historyTriggerPos;
            stutterBuffer.copyFrom(channel, 0, historyBuffer, channel, historyTriggerPos, firstPartSize);
            stutterBuffer.copyFrom(channel, firstPartSize, historyBuffer, channel, 0, lengthToCapture - firstPartSize);
        }
    }
}

Params::RateOption GlitchEngine::selectRandomRate()
{
    // Rates for beat-repeat (1/4, 1/8, or 1/16 note most musically useful)
    Params::RateOption rates[] = {
        Params::RATE_1_4,
        Params::RATE_1_8,
        Params::RATE_1_16
    };

    // Choose one randomly, weighted toward shorter durations
    // 1/4: 20%, 1/8: 40%, 1/16: 40% chance
    float randomValue = random.nextFloat();

    if (randomValue < 0.2f) {
        return rates[0]; // 1/4 note
    } else if (randomValue < 0.6f) {
        return rates[1]; // 1/8 note
    } else {
        return rates[2]; // 1/16 note
    }
}