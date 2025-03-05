//
// Created by Shaked Melman on 05/03/2025.
//

#ifndef JAMMER_STUTTER_H
#define JAMMER_STUTTER_H

#include "juce_audio_utils/juce_audio_utils.h"
#include "../Params.h"
#include "../Shared/TimingManager.h"

class Stutter
{
public:
    Stutter(std::shared_ptr<TimingManager> timingManager);
    ~Stutter() = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();
    void applyStutterEffect(juce::AudioBuffer<float>& buffer,
                            std::vector<int> triggerSamplePositions);

    // Setters
    void setSettings(Params::FxSettings s) { settings = s; }
    void setBufferSize(int numSamples)
    {
        currentBufferSize = numSamples;
    }

private:
    // Settings
    Params::FxSettings settings;

    // Timing manager
    std::shared_ptr<TimingManager> timingManager;

    // Stutter effect methods
    bool shouldStutter();
    void copyStutterDataToBuffer(juce::AudioBuffer<float>& tempBuffer,
                                 int numSamples,
                                 int numChannels);
    void applyStutterCrossfade(juce::AudioBuffer<float>& buffer,
                               const juce::AudioBuffer<float>& tempBuffer,
                               int numSamples,
                               int numChannels);
    void updateStutterPosition(int numSamples);
    void startStutterAtPosition(juce::AudioBuffer<float>& buffer,
                                int samplePosition,
                                int numSamples,
                                int numChannels);
    void applyImmediateStutterEffect(juce::AudioBuffer<float>& buffer,
                                     int samplePosition,
                                     int numSamples,
                                     int numChannels);
    void endStutterEffect();
    void resetStutterState();
    void processActiveStutter(juce::AudioBuffer<float>& buffer,
                              int numSamples,
                              int numChannels);
    void checkAndStartNewStutter(juce::AudioBuffer<float>& buffer,
                                 const juce::MidiBuffer& midiMessages,
                                 int numSamples,
                                 int numChannels);
    void handleTransportLoopDetection();

    // Buffer management
    void addToHistory(const juce::AudioBuffer<float>& buffer);
    void captureFromHistory(int triggerSamplePosition, int lengthToCapture);
    void captureHistorySegment(int historyTriggerPos, int lengthToCapture);

    // Beat-repeat effect state
    bool isStuttering {false};
    int stutterPosition {0};
    int stutterLength {0};
    int stutterRepeatCount {0};
    int stutterRepeatsTotal {2};

    // Selects a random musical rate for repeat duration
    Params::RateOption selectRandomRate();

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

#endif //JAMMER_STUTTER_H
