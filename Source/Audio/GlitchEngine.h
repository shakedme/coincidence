#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "TimingManager.h"

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
     * Processes an audio buffer, applying beat-repeat effect when triggered.
     * Now accepts MIDI messages to allow precise alignment with note events.
     */
    void processAudio(juce::AudioBuffer<float>& buffer,
                      juce::AudioPlayHead* playHead,
                      const juce::MidiBuffer& midiMessages);

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
    int calculateStutterLength(
        const juce::Optional<juce::AudioPlayHead::PositionInfo>& posInfo);

    // Finds the nearest grid position for timing
    double findNearestGridPosition(
        const juce::Optional<juce::AudioPlayHead::PositionInfo>& posInfo);

    // Selects a random musical rate for repeat duration
    Params::RateOption selectRandomRate();

    // Adds audio to the history buffer
    void addToHistory(const juce::AudioBuffer<float>& buffer);

    // Captures a segment from history buffer starting at a specific position
    void captureFromHistory(int triggerSamplePosition, int lengthToCapture);

    // Parameter for stutter probability (0-100%)
    std::atomic<float> stutterProbability {0.0f};

    // Audio processing state
    double sampleRate {44100.0};
    int bufferSize {512};

    // Beat-repeat effect state
    bool isStuttering {false};
    int stutterPosition {0};
    int stutterLength {0};
    int stutterRepeatCount {0};
    int stutterRepeatsTotal {2};

    // Buffer for storing audio segments for beat-repeat effect
    juce::AudioBuffer<float> stutterBuffer;

    // History buffer to store recent audio for accurate beat repeating
    juce::AudioBuffer<float> historyBuffer;
    int historyWritePosition {0};
    int historyBufferSize {0};

    // Most recent buffer size for reference
    int currentBufferSize {0};

    // Random number generator
    juce::Random random;
};