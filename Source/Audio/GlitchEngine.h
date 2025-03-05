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

    // Main processing methods
    void updateTimingInfo(juce::AudioPlayHead* playHead);
    void handleTransportLoopDetection();
    void processActiveStutter(juce::AudioBuffer<float>& buffer, int numSamples, int numChannels);
    void checkAndStartNewStutter(juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midiMessages, int numSamples, int numChannels);

    // Stutter effect helpers
    void copyStutterDataToBuffer(juce::AudioBuffer<float>& tempBuffer, int numSamples, int numChannels);
    void applyStutterCrossfade(juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<float>& tempBuffer, int numSamples, int numChannels);
    void updateStutterPosition(int numSamples);
    void endStutterEffect();
    void resetStutterState();

    // MIDI and triggering
    void checkForMidiTriggers(juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midiMessages, int numSamples, int numChannels);
    void startStutterAtPosition(juce::AudioBuffer<float>& buffer, int samplePosition, int numSamples, int numChannels);
    void applyImmediateStutterEffect(juce::AudioBuffer<float>& buffer, int samplePosition, int numSamples, int numChannels);

    // Buffer management
    void addToHistory(const juce::AudioBuffer<float>& buffer);
    void captureFromHistory(int triggerSamplePosition, int lengthToCapture);
    void captureHistorySegment(int historyTriggerPos, int lengthToCapture);

    // Selects a random musical rate for repeat duration
    Params::RateOption selectRandomRate();

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