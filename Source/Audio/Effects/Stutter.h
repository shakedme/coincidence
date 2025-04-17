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
#include "../../Shared/Parameters/Params.h"
#include "../../Shared/Parameters/StructParameter.h"


class Stutter : public BaseEffect {
public:
    Stutter();

    ~Stutter() override = default;

    void initialize(PluginProcessor &p) override;

    void prepare(const juce::dsp::ProcessSpec &spec) override;

    void process(const juce::dsp::ProcessContextReplacing<float> &context) override;

    void reset() override;

    void setMidiMessages(const juce::MidiBuffer &messages) { midiMessages = messages; }

private:
    std::unique_ptr<Parameter<float>> stutterProbability;

    juce::MidiBuffer midiMessages;

    // Beat-repeat effect state
    bool isStuttering{false};
    int stutterPosition{0};
    int stutterLength{0};
    int stutterRepeatCount{0};
    int stutterRepeatsTotal{2};

    // Buffer for storing audio segments for beat-repeat effect
    juce::AudioBuffer<float> stutterBuffer;

    // History buffer to store recent audio for accurate beat repeating
    juce::AudioBuffer<float> historyBuffer;
    int historyWritePosition{0};
    int historyBufferSize{0};

    // Random number generator
    juce::Random random;

    // Stutter effect methods
    bool shouldStutter();

    // Block-based processing methods (more efficient)
    void addToHistoryFromBlock(const juce::dsp::AudioBlock<const float> &block);

    void processActiveStutterBlock(juce::dsp::AudioBlock<float> &outBlock, int numSamples, int numChannels);

    void startStutterAtPositionBlock(juce::dsp::AudioBlock<float> &outBlock, juce::int64 samplePosition, int numSamples,
                                     int numChannels);

    // Buffer-based methods (legacy)
    void copyStutterDataToBuffer(juce::AudioBuffer<float> &tempBuffer,
                                 int numSamples,
                                 int numChannels);

    void applyStutterCrossfade(juce::AudioBuffer<float> &buffer,
                               const juce::AudioBuffer<float> &tempBuffer,
                               int numSamples,
                               int numChannels);

    void updateStutterPosition(int numSamples);

    void startStutterAtPosition(juce::AudioBuffer<float> &buffer,
                                juce::int64 samplePosition,
                                int numSamples,
                                int numChannels);

    void applyImmediateStutterEffect(juce::AudioBuffer<float> &buffer,
                                     juce::int64 samplePosition,
                                     int numSamples,
                                     int numChannels);

    void endStutterEffect();

    void resetStutterState();

    void handleTransportLoopDetection();

    void captureFromHistory(juce::int64 triggerSamplePosition, int lengthToCapture);

    std::vector<juce::int64> checkForMidiTriggers(const juce::MidiBuffer &midiMessages);

    // Selects a random musical rate for repeat duration
    Models::RateOption selectRandomRate();
};

#endif //JAMMER_STUTTER_H
