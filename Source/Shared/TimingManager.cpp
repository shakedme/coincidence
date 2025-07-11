#include "TimingManager.h"
#include "Parameters/Params.h"

using namespace Params;
using namespace Models;

TimingManager::TimingManager() {
    // Initialize member variables to default values
    for (int i = 0; i < Models::NUM_RATE_OPTIONS; i++) {
        lastTriggerTimes[i] = 0.0;
    }
}

void TimingManager::prepareToPlay(double liveSampleRate) {
    this->sampleRate = liveSampleRate;
    samplePosition = 0;

    // Reset timing variables
    ppqPosition = 0.0;
    lastPpqPosition = 0.0;
    lastContinuousPpqPosition = 0.0;

    // Reset trigger times
    for (int i = 0; i < Models::NUM_RATE_OPTIONS; i++) {
        lastTriggerTimes[i] = 0.0;
    }

    loopJustDetected = false;
}

void TimingManager::updateTimingInfo(juce::AudioPlayHead *playHead) {
    // Store the previous ppq position
    lastPpqPosition = ppqPosition;
    lastContinuousPpqPosition = ppqPosition; // Save before updates

    // Get current playhead information
    if (playHead != nullptr) {
        juce::Optional<juce::AudioPlayHead::PositionInfo> posInfo =
                playHead->getPosition();

        if (posInfo.hasValue()) {
            if (posInfo->getBpm().hasValue())
                bpm = *posInfo->getBpm();

            if (posInfo->getPpqPosition().hasValue()) {
                ppqPosition = *posInfo->getPpqPosition();

                // Detect a loop - PPQ position has jumped backward significantly
                // Small jumps backward (less than a quarter note) could be jitter, ignore those
                if (ppqPosition < lastContinuousPpqPosition - 0.25) {
                    loopJustDetected = true;

                    // Reset timing state for all rates
                    for (int i = 0; i < Models::NUM_RATE_OPTIONS; i++) {
                        lastTriggerTimes[i] = 0.0;
                    }
                } else {
                    loopJustDetected = false;
                }
            }
        }
    }
}

void TimingManager::updateSamplePosition(int numSamples) {
    samplePosition += numSamples;
}

void TimingManager::updateLastTriggerTime(Models::RateOption rate, double triggerTime) {
    lastTriggerTimes[static_cast<int>(rate)] = triggerTime;
}

double TimingManager::getNextExpectedGridPoint(Models::RateOption selectedRate,

                                               int rateIndex) {
    double durationInQuarters = getDurationInQuarters(selectedRate);
    double lastTriggerTime = getLastTriggerTimes()[rateIndex];

    if (wasLoopDetected() || lastTriggerTime <= 0.0) {
        // At loop points, align with the closest grid
        double gridStartPpq =
                std::floor(ppqPosition / durationInQuarters) * durationInQuarters;

        // If we're very close to a grid point, use that
        double ppqSinceGrid = ppqPosition - gridStartPpq;
        double triggerWindowInPPQ = 0.05 * std::max(1.0, bpm / 120.0);
        return ppqSinceGrid < triggerWindowInPPQ ? gridStartPpq
                                                 : gridStartPpq + durationInQuarters;
    }
    // Calculate how many grid units have passed since the last trigger
    double gridsSinceLastTrigger =
            std::floor((ppqPosition - lastTriggerTime) / durationInQuarters);

    // The next grid point should be exactly on a grid division from the last trigger
    return lastTriggerTime + ((gridsSinceLastTrigger + 1) * durationInQuarters);
}

double TimingManager::getDurationInQuarters(Models::RateOption rate
) {
    double durationInQuarters;
    switch (rate) {
        case Models::RATE_1_1:
            durationInQuarters = 4.0;
            break;
        case Models::RATE_1_2:
            durationInQuarters = 2.0;
            break;
        case Models::RATE_1_4:
            durationInQuarters = 1.0;
            break;
        case Models::RATE_1_8:
            durationInQuarters = 0.5;
            break;
        case Models::RATE_1_16:
            durationInQuarters = 0.25;
            break;
        case Models::RATE_1_32:
            durationInQuarters = 0.125;
            break;
        default:
            durationInQuarters = 1.0;
            break;
    }

    return durationInQuarters;
}

bool TimingManager::shouldTriggerNote(Models::RateOption rate) {
    // Calculate the duration in quarter notes
    double durationInQuarters = getDurationInQuarters(rate);
    int rateIndex = static_cast<int>(rate);

    // If we just detected a loop or it's the first trigger for this rate
    if (loopJustDetected || lastTriggerTimes[rateIndex] <= 0.0) {
        // Find the closest grid point at or before current position
        double gridStartPpq =
                std::floor(ppqPosition / durationInQuarters) * durationInQuarters;

        // Calculate a reasonable window for triggering (adaptive to tempo)
        double triggerWindowInPPQ = 0.05 * std::max(1.0, bpm / 120.0);

        // If we're close to a grid point at the start of the loop, we should trigger
        double ppqSinceGrid = ppqPosition - gridStartPpq;

        // Trigger if we're very close to a grid point
        if (ppqSinceGrid < triggerWindowInPPQ) {
            return true;
        }

        // Trigger if the next grid point falls within this buffer
        double nextGridPpq = gridStartPpq + durationInQuarters;
        int blockSize = 1024; // Typical buffer size
        double ppqSpanOfCurrentBuffer = (blockSize / sampleRate) * (bpm / 60.0);
        double ppqUntilNextGrid = nextGridPpq - ppqPosition;

        if (ppqUntilNextGrid >= 0 && ppqUntilNextGrid <= ppqSpanOfCurrentBuffer) {
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
    double gridsSinceLastTrigger =
            std::floor((ppqPosition - lastTriggerTime) / durationInQuarters);

    // Calculate the next grid point after our last trigger
    nextGridPoint = lastTriggerTime + ((gridsSinceLastTrigger + 1) * durationInQuarters);

    // Calculate a narrow window for triggering (adaptive to tempo)
    double triggerWindowInPPQ = 0.01 * std::max(1.0, bpm / 120.0);

    // Check if the next grid point is within this buffer
    double ppqUntilNextGrid = nextGridPoint - ppqPosition;
    int blockSize = 1024; // Typical buffer size
    double ppqSpanOfCurrentBuffer = (blockSize / sampleRate) * (bpm / 60.0);

    // Grid point is coming up in this buffer
    if (ppqUntilNextGrid >= 0 && ppqUntilNextGrid <= ppqSpanOfCurrentBuffer) {
        return true;
    }

    // We already passed the grid point slightly (timing issue)
    if (ppqUntilNextGrid < 0 && ppqUntilNextGrid > -triggerWindowInPPQ) {
        return true;
    }

    return false;
}

double TimingManager::getNoteDurationInSamples(Models::RateOption rate) {
    // Calculate duration in quarter notes
    double quarterNotesPerSecond = bpm / 60.0;
    double secondsPerQuarterNote = 1.0 / quarterNotesPerSecond;

    double durationInQuarters = getDurationInQuarters(rate);
    double durationInSeconds = secondsPerQuarterNote * durationInQuarters;
    double durationInSamples = durationInSeconds * sampleRate;

    // Return the duration, ensuring it's at least one sample
    return juce::jmax(1.0, durationInSamples);
}