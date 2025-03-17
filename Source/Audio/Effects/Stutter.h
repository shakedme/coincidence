//
// Created by Shaked Melman on 05/03/2025.
//

#ifndef JAMMER_STUTTER_H
#define JAMMER_STUTTER_H

#include "juce_audio_utils/juce_audio_utils.h"
#include "../../Shared/Models.h"
#include "../../Shared/TimingManager.h"
#include "../Sampler/SampleManager.h"
#include "BaseEffect.h"

class Stutter : public BaseEffect
{
public:
    Stutter(PluginProcessor& processor);
    ~Stutter() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void applyStutterEffect(juce::AudioBuffer<float>& buffer,
                            std::vector<juce::int64> triggerSamplePositions);

private:
    std::unique_ptr<AppState::ParameterBinding<Models::StutterSettings>> paramBinding;
    Models::StutterSettings settings;

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

    // Random number generator
    juce::Random random;

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
                                juce::int64 samplePosition,
                                int numSamples,
                                int numChannels);
    void applyImmediateStutterEffect(juce::AudioBuffer<float>& buffer,
                                     juce::int64 samplePosition,
                                     int numSamples,
                                     int numChannels);
    void endStutterEffect();
    void resetStutterState();
    void processActiveStutter(juce::AudioBuffer<float>& buffer,
                              int numSamples,
                              int numChannels);
    void handleTransportLoopDetection();

    // Buffer management
    void addToHistory(const juce::AudioBuffer<float>& buffer);
    void captureFromHistory(juce::int64 triggerSamplePosition, int lengthToCapture);
    void captureHistorySegment(int historyTriggerPos, int lengthToCapture);

    // Selects a random musical rate for repeat duration
    Models::RateOption selectRandomRate();
};

#endif //JAMMER_STUTTER_H
