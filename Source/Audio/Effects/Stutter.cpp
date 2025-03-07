//
// Created by Shaked Melman on 05/03/2025.
//

#include "Stutter.h"

Stutter::Stutter(std::shared_ptr<TimingManager> timingManager)
    : timingManager(timingManager)
{
    // Initialize random generator
    random.setSeedRandomly();
}

void Stutter::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentBufferSize = samplesPerBlock;

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

void Stutter::releaseResources()
{
    // Reset state
    isStuttering = false;

    // Clear buffers
    stutterBuffer.clear();
    historyBuffer.clear();
}

void Stutter::applyStutterEffect(juce::AudioBuffer<float>& buffer,
                                 std::vector<juce::int64> triggerSamplePositions)
{
    // Store buffer info for later use
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    addToHistory(buffer);
    handleTransportLoopDetection();

//     Process stutter effect
    if (isStuttering)
    {
        processActiveStutter(buffer, numSamples, numChannels);
    }
    else if (shouldStutter() && !triggerSamplePositions.empty())
    {
        startStutterAtPosition(
            buffer, triggerSamplePositions[0], numSamples, numChannels);
    }
}

bool Stutter::shouldStutter()
{
    return settings.stutterProbability > 0.0f
           && random.nextFloat() < (settings.stutterProbability / 100.0f);
}

void Stutter::processActiveStutter(juce::AudioBuffer<float>& buffer,
                                   int numSamples,
                                   int numChannels)
{
    // Safety check for division by zero
    if (stutterLength <= 0)
    {
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
}

void Stutter::copyStutterDataToBuffer(juce::AudioBuffer<float>& tempBuffer,
                                      int numSamples,
                                      int numChannels)
{
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
}

void Stutter::applyStutterCrossfade(juce::AudioBuffer<float>& buffer,
                                    const juce::AudioBuffer<float>& tempBuffer,
                                    int numSamples,
                                    int numChannels)
{
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
}

void Stutter::updateStutterPosition(int numSamples)
{
    int oldPosition = stutterPosition;
    stutterPosition = (stutterPosition + numSamples) % stutterLength;

    // Log position wraparound
    if (stutterPosition < oldPosition)
    {
        // We've completed a full loop through the stutter buffer
        stutterRepeatCount++;
        if (stutterRepeatCount >= stutterRepeatsTotal)
        {
            endStutterEffect();
        }
    }

    // Safety timeout - prevent stutters from lasting too long
    if (stutterRepeatCount > 8) // Maximum number of repeats to prevent hanging
    {
        resetStutterState();
    }
}

void Stutter::endStutterEffect()
{
    // We will apply the crossfade when actually ending the stutter in processActiveStutter
    // This just marks that we should end the stutter
    isStuttering = false;
    stutterPosition = 0;
    stutterLength = 0;
    stutterRepeatCount = 0;
    stutterRepeatsTotal = 0;
}

void Stutter::resetStutterState()
{
    isStuttering = false;
    stutterPosition = 0;
    stutterLength = 0;
    stutterRepeatCount = 0;
    stutterRepeatsTotal = 0;
}

void Stutter::startStutterAtPosition(juce::AudioBuffer<float>& buffer,
                                     juce::int64 samplePosition,
                                     int numSamples,
                                     int numChannels)
{
    // Choose a rate (1/8, 1/16 note, etc.)
    Params::RateOption selectedRate = selectRandomRate();

    // Calculate stutter length based on musical timing
    Params::GeneratorSettings settings;
    int captureLength =
        static_cast<int>(timingManager->getNoteDurationInSamples(selectedRate, settings));

//    // Limit to a reasonable value
    captureLength = juce::jmin(captureLength, stutterBuffer.getNumSamples());

    // Capture the musical segment starting from the note position
    captureFromHistory(samplePosition, captureLength);
//
//    // Configure stutter parameters
    isStuttering = true;
    stutterLength = captureLength;
    stutterPosition = 0;
    stutterRepeatsTotal = 2 + random.nextInt(2); // 2-4 repeats
    stutterRepeatCount = 0;

//    // Apply stutter effect immediately for the rest of this buffer
    applyImmediateStutterEffect(buffer, samplePosition, numSamples, numChannels);
}

void Stutter::applyImmediateStutterEffect(juce::AudioBuffer<float>& buffer,
                                          juce::int64 samplePosition,
                                          int numSamples,
                                          int numChannels)
{
    juce::AudioBuffer<float> tempBuffer(numChannels, numSamples - samplePosition);
    tempBuffer.clear();

    for (int channel = 0;
         channel < juce::jmin(numChannels, stutterBuffer.getNumChannels());
         ++channel)
    {
        float* channelData = tempBuffer.getWritePointer(channel);
        const float* stutterData = stutterBuffer.getReadPointer(channel);

        for (int i = 0; i < numSamples - samplePosition; ++i)
        {
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

void Stutter::addToHistory(const juce::AudioBuffer<float>& buffer)
{
    // Add the current buffer to the history circular buffer
    int numSamples = buffer.getNumSamples();

    for (int channel = 0;
         channel < juce::jmin(buffer.getNumChannels(), historyBuffer.getNumChannels());
         ++channel)
    {
        if (historyWritePosition + numSamples <= historyBufferSize)
        {
            // Simple copy if it doesn't wrap around
            historyBuffer.copyFrom(
                channel, historyWritePosition, buffer, channel, 0, numSamples);
        }
        else
        {
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

void Stutter::captureFromHistory(juce::int64 triggerSamplePosition, int lengthToCapture)
{
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
    if (stutterBuffer.getNumSamples() < lengthToCapture)
    {
        stutterBuffer.setSize(
            stutterBuffer.getNumChannels(), lengthToCapture, true, true, true);
    }

    // Clear stutter buffer
    stutterBuffer.clear();

    // Copy the segment from history buffer to stutter buffer
    captureHistorySegment(historyTriggerPos, lengthToCapture);
}

void Stutter::captureHistorySegment(int historyTriggerPos, int lengthToCapture)
{
    for (int channel = 0; channel < juce::jmin(stutterBuffer.getNumChannels(),
                                               historyBuffer.getNumChannels());
         ++channel)
    {
        if (historyTriggerPos + lengthToCapture <= historyBufferSize)
        {
            // Simple case - no wrap-around
            stutterBuffer.copyFrom(
                channel, 0, historyBuffer, channel, historyTriggerPos, lengthToCapture);
        }
        else
        {
            // Handle wrap-around case
            int firstPartSize = historyBufferSize - historyTriggerPos;
            stutterBuffer.copyFrom(
                channel, 0, historyBuffer, channel, historyTriggerPos, firstPartSize);
            stutterBuffer.copyFrom(channel,
                                   firstPartSize,
                                   historyBuffer,
                                   channel,
                                   0,
                                   lengthToCapture - firstPartSize);
        }
    }
}

void Stutter::handleTransportLoopDetection()
{
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
}

Params::RateOption Stutter::selectRandomRate()
{
    Params::RateOption rates[] = {Params::RATE_1_4,
                                  Params::RATE_1_8,
                                  Params::RATE_1_16,
                                  Params::RATE_1_32,
                                  Params::RATE_1_64};

    float randomValue = random.nextFloat();

    if (randomValue < 0.1f)
    {
        return rates[0]; // 1/4 note
    }
    else if (randomValue < 0.35f)
    {
        return rates[1]; // 1/8 note
    }
    else if (randomValue < 0.75f)
    {
        return rates[2]; // 1/16 note
    }
    else if (randomValue < 0.9f)
    {
        return rates[3]; // 1/32 note
    }
    else
    {
        return rates[4]; // 1/64 note
    }
}
