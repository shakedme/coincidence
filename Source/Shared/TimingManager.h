#pragma once

#include "juce_audio_utils/juce_audio_utils.h"
#include "Models.h"
#include "ParameterBinding.h"

/**
 * Class to handle timing-related functionality including BPM, position tracking,
 * note trigger timing, and synchronization with DAW.
 */
class TimingManager {
public:
    TimingManager();

    ~TimingManager() = default;

    // Initialize timing variables
    void prepareToPlay(double sampleRate);

    // Update timing information from playhead
    void updateTimingInfo(juce::AudioPlayHead *playHead);

    // Get current timing information
    double getBpm() const { return bpm; }

    double getPpqPosition() const { return ppqPosition; }

    double getLastPpqPosition() const { return lastPpqPosition; }

    double getSampleRate() const { return sampleRate; }

    juce::int64 getSamplePosition() const { return samplePosition; }

    // Access to the last trigger times array
    const double *getLastTriggerTimes() const { return lastTriggerTimes; }

    // Update sample position after processing a buffer
    void updateSamplePosition(int numSamples);

    // Check if a note should be triggered at the current position for a given rate
    bool shouldTriggerNote(Config::RateOption rate);

    // Calculate the duration in samples for a given rate
    double getNoteDurationInSamples(Config::RateOption rate);

    double getNextExpectedGridPoint(Config::RateOption selectedRate,
                                    int rateIndex);

    // Update the last trigger time for a rate
    void updateLastTriggerTime(Config::RateOption rate, double triggerTime);

    // Check if a loop was just detected
    bool wasLoopDetected() const { return loopJustDetected; }

    // Clear loop detection state
    void clearLoopDetection() { loopJustDetected = false; }

    double getDurationInQuarters(Config::RateOption rate);

private:
    // Timing state
    double sampleRate = 44100.0;
    juce::int64 samplePosition = 0;
    double bpm = 120.0;
    double ppqPosition = 0.0;
    double lastPpqPosition = 0.0;
    double lastTriggerTimes[Config::NUM_RATE_OPTIONS] = {0.0};
    bool loopJustDetected = false;
    double lastContinuousPpqPosition = 0.0;  // For detecting transport loops
};