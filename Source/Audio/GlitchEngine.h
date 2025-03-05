#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "TimingManager.h"

/**
 * Simplified GlitchEngine that provides grid-based stutter effects
 */
class GlitchEngine
{
public:
    GlitchEngine(std::shared_ptr<TimingManager> timingManager);
    ~GlitchEngine();

    /**
     * Prepares the engine for playback with the given sample rate and block size.
     */
    void prepareToPlay(double sampleRate, int samplesPerBlock);

    /**
     * Releases resources when audio processing is no longer needed.
     */
    void releaseResources();

    /**
     * Processes an audio buffer, applying stutter effect when triggered.
     */
    void processAudio(juce::AudioBuffer<float>& buffer,
                      juce::AudioPlayHead* playHead);

    /**
     * Sets the stutter probability (0.0-100.0)
     * Higher values increase the chance of triggering a stutter effect
     */
    void setStutterProbability(float probability) { stutterProbability = probability; }

private:
    std::shared_ptr<TimingManager> timingManager;

    // Determines if we should trigger a stutter based on probability
    bool shouldTriggerStutter();

    // Calculates the musically-relevant stutter length in samples
    int calculateStutterLength(const juce::Optional<juce::AudioPlayHead::PositionInfo>& posInfo);

    // Finds the nearest grid position for timing
    double findNearestGridPosition(const juce::Optional<juce::AudioPlayHead::PositionInfo>& posInfo);

    // Parameter for stutter probability (0-100%)
    std::atomic<float> stutterProbability{0.0f};

    // Audio processing state
    double sampleRate{44100.0};
    int bufferSize{512};

    // Stutter effect state
    bool isStuttering{false};
    int stutterPosition{0};
    int stutterLength{0};
    int stutterRepeatCount{0};
    int stutterRepeatsTotal{2};

    // Buffer for storing audio segments for stutter effect
    juce::AudioBuffer<float> stutterBuffer;

    // Random number generator
    juce::Random random;
};