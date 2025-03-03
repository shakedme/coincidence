#include "TimingManager.h"

TimingManager::TimingManager()
{
    // Initialize member variables to default values
    for (int i = 0; i < Params::NUM_RATE_OPTIONS; i++)
    {
        lastTriggerTimes[i] = 0.0;
    }
}

void TimingManager::prepareToPlay(double sampleRate)
{
    this->sampleRate = sampleRate;
    samplePosition = 0;

    // Reset timing variables
    ppqPosition = 0.0;
    lastPpqPosition = 0.0;
    lastContinuousPpqPosition = 0.0;

    // Reset trigger times
    for (int i = 0; i < Params::NUM_RATE_OPTIONS; i++)
    {
        lastTriggerTimes[i] = 0.0;
    }
    
    loopJustDetected = false;
}

void TimingManager::updateTimingInfo(juce::AudioPlayHead* playHead)
{
    // Store the previous ppq position
    lastPpqPosition = ppqPosition;
    lastContinuousPpqPosition = ppqPosition; // Save before updates

    // Get current playhead information
    if (playHead != nullptr)
    {
        juce::Optional<juce::AudioPlayHead::PositionInfo> posInfo =
            playHead->getPosition();

        if (posInfo.hasValue())
        {
            if (posInfo->getBpm().hasValue())
                bpm = *posInfo->getBpm();

            if (posInfo->getPpqPosition().hasValue())
            {
                ppqPosition = *posInfo->getPpqPosition();

                // Detect a loop - PPQ position has jumped backward significantly
                // Small jumps backward (less than a quarter note) could be jitter, ignore those
                if (ppqPosition < lastContinuousPpqPosition - 0.25)
                {
                    loopJustDetected = true;

                    // Reset timing state for all rates
                    for (int i = 0; i < Params::NUM_RATE_OPTIONS; i++)
                    {
                        lastTriggerTimes[i] = 0.0;
                    }
                }
                else
                {
                    loopJustDetected = false;
                }
            }
        }
    }
}

void TimingManager::updateSamplePosition(int numSamples)
{
    samplePosition += numSamples;
}

void TimingManager::updateLastTriggerTime(Params::RateOption rate, double triggerTime)
{
    lastTriggerTimes[static_cast<int>(rate)] = triggerTime;
}

bool TimingManager::shouldTriggerNote(Params::RateOption rate, const Params::GeneratorSettings& settings)
{
    // Calculate the duration in quarter notes
    double durationInQuarters;
    switch (rate)
    {
        case Params::RATE_1_2:
            durationInQuarters = 2.0;
            break;
        case Params::RATE_1_4:
            durationInQuarters = 1.0;
            break;
        case Params::RATE_1_8:
            durationInQuarters = 0.5;
            break;
        case Params::RATE_1_16:
            durationInQuarters = 0.25;
            break;
        case Params::RATE_1_32:
            durationInQuarters = 0.125;
            break;
        default:
            durationInQuarters = 1.0;
            break;
    }

    // Apply rhythm mode modifications
    switch (settings.rhythmMode)
    {
        case Params::RHYTHM_DOTTED:
            durationInQuarters *= 1.5;
            break;
        case Params::RHYTHM_TRIPLET:
            durationInQuarters *= 2.0 / 3.0;
            break;
        default:
            break;
    }

    int rateIndex = static_cast<int>(rate);

    // If we just detected a loop or it's the first trigger for this rate
    if (loopJustDetected || lastTriggerTimes[rateIndex] <= 0.0)
    {
        // Find the closest grid point at or before current position
        double gridStartPpq = std::floor(ppqPosition / durationInQuarters) * durationInQuarters;

        // Calculate a reasonable window for triggering (adaptive to tempo)
        double triggerWindowInPPQ = 0.05 * std::max(1.0, bpm / 120.0);

        // If we're close to a grid point at the start of the loop, we should trigger
        double ppqSinceGrid = ppqPosition - gridStartPpq;

        // Trigger if we're very close to a grid point
        if (ppqSinceGrid < triggerWindowInPPQ)
        {
            return true;
        }

        // Trigger if the next grid point falls within this buffer
        double nextGridPpq = gridStartPpq + durationInQuarters;
        int blockSize = 1024; // Typical buffer size
        double ppqSpanOfCurrentBuffer = (blockSize / sampleRate) * (bpm / 60.0);
        double ppqUntilNextGrid = nextGridPpq - ppqPosition;

        if (ppqUntilNextGrid >= 0 && ppqUntilNextGrid <= ppqSpanOfCurrentBuffer)
        {
            return true;
        }

        return false;
    }

    // Normal case - not at a loop point
    // Find the next grid point that should trigger a note
    double lastTriggerTime = lastTriggerTimes[rateIndex];
    
    // Calculate the next expected grid point
    double nextGridPoint = 0.0;
    
    // Calculate the number of whole grid units since the last trigger
    double gridsSinceLastTrigger = std::floor((ppqPosition - lastTriggerTime) / durationInQuarters);
    
    // Calculate the next grid point after our last trigger
    nextGridPoint = lastTriggerTime + ((gridsSinceLastTrigger + 1) * durationInQuarters);
    
    // Calculate a narrow window for triggering (adaptive to tempo)
    double triggerWindowInPPQ = 0.01 * std::max(1.0, bpm / 120.0);
    
    // Check if the next grid point is within this buffer
    double ppqUntilNextGrid = nextGridPoint - ppqPosition;
    int blockSize = 1024; // Typical buffer size
    double ppqSpanOfCurrentBuffer = (blockSize / sampleRate) * (bpm / 60.0);
    
    // Grid point is coming up in this buffer
    if (ppqUntilNextGrid >= 0 && ppqUntilNextGrid <= ppqSpanOfCurrentBuffer)
    {
        return true;
    }
    
    // We already passed the grid point slightly (timing issue)
    if (ppqUntilNextGrid < 0 && ppqUntilNextGrid > -triggerWindowInPPQ)
    {
        return true;
    }
    
    return false;
}

double TimingManager::getNoteDurationInSamples(Params::RateOption rate, const Params::GeneratorSettings& settings)
{
    // Calculate duration in quarter notes
    double quarterNotesPerSecond = bpm / 60.0;
    double secondsPerQuarterNote = 1.0 / quarterNotesPerSecond;

    double durationInQuarters;

    switch (rate)
    {
        case Params::RATE_1_2:
            durationInQuarters = 2.0;
            break; // Half note
        case Params::RATE_1_4:
            durationInQuarters = 1.0;
            break; // Quarter note
        case Params::RATE_1_8:
            durationInQuarters = 0.5;
            break; // Eighth note
        case Params::RATE_1_16:
            durationInQuarters = 0.25;
            break; // Sixteenth note
        case Params::RATE_1_32:
            durationInQuarters = 0.125;
            break; // Thirty-second note
        default:
            durationInQuarters = 1.0;
            break;
    }

    // Apply rhythm mode modifications
    switch (settings.rhythmMode)
    {
        case Params::RHYTHM_DOTTED:
            durationInQuarters *= 1.5; // Dotted note = 1.5x the normal duration
            break;
        case Params::RHYTHM_TRIPLET:
            durationInQuarters *= 2.0 / 3.0; // Triplet = 2/3 the normal duration
            break;
        case Params::RHYTHM_NORMAL:
        default:
            // No modification for normal rhythm
            break;
    }

    double durationInSeconds = secondsPerQuarterNote * durationInQuarters;
    double durationInSamples = durationInSeconds * sampleRate;

    // Return the duration, ensuring it's at least one sample
    return juce::jmax(1.0, durationInSamples);
} 