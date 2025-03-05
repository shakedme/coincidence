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
}

void GlitchEngine::processAudio(juce::AudioBuffer<float>& buffer,
                                juce::AudioPlayHead* playHead)
{
    // Get timing info from playhead
    juce::Optional<juce::AudioPlayHead::PositionInfo> posInfo;

    if (playHead != nullptr)
    {
        timingManager->updateTimingInfo(playHead);
    }

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // Update sample position in timing manager
    timingManager->updateSamplePosition(numSamples);

    // Check if we've detected a loop in the transport (from TimingManager)
    if (timingManager->wasLoopDetected())
    {
        // Reset stuttering state when transport loops
        isStuttering = false;
        stutterPosition = 0;
        stutterLength = 0;
        stutterRepeatCount = 0;

        // Clear the loop detection flag
        timingManager->clearLoopDetection();
    }

    // If not currently stuttering, check if we should start
    if (!isStuttering && stutterProbability > 0.0f)
    {
        if (shouldTriggerStutter())
        {
            // Create a temporary settings object for use with the timing manager
            Params::GeneratorSettings settings;

            // Use TimingManager to determine if we're at a grid-aligned position
            bool shouldTrigger = false;

            // Try different rate options to see if any should trigger
            Params::RateOption rateOptions[] = {
                Params::RATE_1_4, Params::RATE_1_8, Params::RATE_1_16};

            Params::RateOption selectedRate = Params::RATE_1_4; // Default

            for (auto rate: rateOptions)
            {
                if (timingManager->shouldTriggerNote(rate, settings))
                {
                    shouldTrigger = true;
                    selectedRate = rate;
                    break;
                }
            }

            if (shouldTrigger)
            {
                // We're at a grid position, start stuttering
                isStuttering = true;
                stutterPosition = 0;

                // Use the timing manager to calculate stutter length based on musical timing
                stutterLength = static_cast<int>(
                    timingManager->getNoteDurationInSamples(selectedRate, settings));

                // Determine number of repeats (2-4)
                stutterRepeatsTotal = 2 + random.nextInt(3); // 2 to 4
                stutterRepeatCount = 0;

                // Make sure stutterLength doesn't exceed the stutter buffer size
                stutterLength = juce::jmin(stutterLength, stutterBuffer.getNumSamples());

                // Copy current buffer to stutter buffer for looping
                for (int channel = 0;
                     channel < juce::jmin(numChannels, stutterBuffer.getNumChannels());
                     ++channel)
                {
                    // First, copy what we have from the current buffer
                    stutterBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);
                }

                // Update last trigger time in the timing manager
                timingManager->updateLastTriggerTime(selectedRate,
                                                     timingManager->getPpqPosition());
            }
        }
    }

    // If we're stuttering, replace audio with the stutter buffer
    if (isStuttering)
    {
        // Create a temporary buffer for mixing
        juce::AudioBuffer<float> tempBuffer(numChannels, numSamples);
        tempBuffer.clear();

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
        stutterPosition = (stutterPosition + numSamples) % stutterLength;

        // Check if we should finish this stutter instance
        if (stutterPosition < numSamples)
        {
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

                // End the stutter
                isStuttering = false;
            }
        }
    }
    // If not stuttering, the original audio in buffer passes through unchanged
}

bool GlitchEngine::shouldTriggerStutter()
{
    // Get current stutter probability
    float probability = stutterProbability.load();

    // If probability is zero, never trigger
    if (probability <= 0.0f)
        return false;

    // We'll leverage the current timing position to make stutters more musical
    // Only trigger stutters with a certain probability, weighted by proximity to musical grid positions

    // Basic probability calculation
    float baseProb = probability / 100.0f;

    // Add some randomness - we don't want every beat to stutter
    float randomFactor = random.nextFloat();

    // Use our randomness, but weight it based on how close we are to a significant beat boundary
    // This gives a more musical feel to the stutters
    double ppqPosition = timingManager->getPpqPosition();
    double lastPpqPosition = timingManager->getLastPpqPosition();

    // Check if we're close to a beat boundary (whole, half, quarter notes)
    double distToWholeBeat = std::abs(ppqPosition - std::floor(ppqPosition));
    double distToHalfBeat = std::abs(ppqPosition - (std::floor(ppqPosition * 2.0) / 2.0));
    double distToQuarterBeat =
        std::abs(ppqPosition - (std::floor(ppqPosition * 4.0) / 4.0));

    // The closest we are to a significant boundary, the more likely we trigger
    double closestBoundaryDist =
        std::min({distToWholeBeat, distToHalfBeat, distToQuarterBeat});

    // Normalize to 0-1 range (0 = on beat, 1 = furthest from beat)
    double normalizedDist =
        closestBoundaryDist * 4.0; // 4.0 = 1/16th note is max distance
    normalizedDist = std::min(normalizedDist, 1.0);

    // Calculate musical weight (higher when closer to beat)
    float musicalWeight = 1.0f - static_cast<float>(normalizedDist);

    // Final probability is base probability * random factor * musical weight
    float finalProb = baseProb * musicalWeight;

    // Adjust to account for buffer size (important for lower sample rates)
    float adjustedProb = finalProb * bufferSize / (sampleRate / 4.0f);

    // Limit max probability to avoid too frequent stutters
    adjustedProb = juce::jmin(adjustedProb, 0.5f);

    // Random decision
    return randomFactor < adjustedProb;
}

int GlitchEngine::calculateStutterLength(
    const juce::Optional<juce::AudioPlayHead::PositionInfo>& posInfo)
{
    // Create a temporary settings object for use with timing manager
    Params::GeneratorSettings settings;

    // Choose a random rhythm mode (normal, dotted, or triplet)
    const int rhythmModes[] = {
        Params::RHYTHM_NORMAL, Params::RHYTHM_DOTTED, Params::RHYTHM_TRIPLET};

    settings.rhythmMode = static_cast<Params::RhythmMode>(rhythmModes[random.nextInt(3)]);

    // Choose a random note division (1/4, 1/8, or 1/16 note)
    const Params::RateOption divisions[] = {
        Params::RATE_1_4, Params::RATE_1_8, Params::RATE_1_16};

    Params::RateOption selectedRate = divisions[random.nextInt(3)];

    // Use timing manager to calculate stutter length in samples
    double stutterLengthSamples =
        timingManager->getNoteDurationInSamples(selectedRate, settings);

    // Return the stutter length as an integer
    return static_cast<int>(stutterLengthSamples);
}

double GlitchEngine::findNearestGridPosition(
    const juce::Optional<juce::AudioPlayHead::PositionInfo>& posInfo)
{
    // This method is kept for backwards compatibility, but we should prefer
    // using the TimingManager's grid position detection instead.

    // We can forward this to the TimingManager or implement a simplified version
    // Just return the current PPQ position if we don't have valid timing info
    if (!posInfo->getPpqPosition().hasValue() || !posInfo->getBpm().hasValue())
        return 0.0;

    // Get current PPQ position
    double currentPpq = *posInfo->getPpqPosition();

    // For simple quantization to quarter notes:
    double quarterNoteGridPos =
        std::floor(currentPpq) + 0.0; // Nearest quarter note behind
    double eighthNoteGridPos =
        std::floor(currentPpq * 2.0) / 2.0; // Nearest eighth note behind
    double sixteenthNoteGridPos =
        std::floor(currentPpq * 4.0) / 4.0; // Nearest sixteenth note behind

    // Find the closest of these positions
    double distToQuarter = std::abs(currentPpq - quarterNoteGridPos);
    double distToEighth = std::abs(currentPpq - eighthNoteGridPos);
    double distToSixteenth = std::abs(currentPpq - sixteenthNoteGridPos);

    if (distToQuarter <= distToEighth && distToQuarter <= distToSixteenth)
        return quarterNoteGridPos;
    else if (distToEighth <= distToSixteenth)
        return eighthNoteGridPos;
    else
        return sixteenthNoteGridPos;
}