#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "MidiGeneratorParams.h"
#include "SampleManager.h"

// Forward declarations
class MidiGeneratorEditor;
class MidiGeneratorLookAndFeel;

/**
 * MidiGeneratorProcessor - Main processor class for the MIDI generator plugin
 */
class MidiGeneratorProcessor
    : public juce::AudioProcessor
    , private juce::Timer
{
public:
    //==============================================================================
    MidiGeneratorProcessor();
    ~MidiGeneratorProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Timer callback for any background tasks
    void timerCallback() override;
    
    // MIDI note state tracking
    bool isNoteActive() const { return noteIsActive; }

    // Parameters
    juce::AudioProcessorValueTreeState parameters;
    
    // Sample management
    void addSample(const juce::File& file);
    void removeSample(int index);
    void clearAllSamples();
    void selectSample(int index);
    int getNumSamples() const;
    juce::String getSampleName(int index) const;
    SampleManager& getSampleManager() { return sampleManager; }
    
    // Current state values for UI visualization
    float getCurrentRandomizedGate() const { return currentRandomizedGate; }
    float getCurrentRandomizedVelocity() const { return currentRandomizedVelocity; }
    
    // Helper methods for UI
    juce::String getRhythmModeText(MidiGeneratorParams::RhythmMode mode) const;

private:
    //==============================================================================
    // Sample management
    SampleManager sampleManager;

    // Plugin state
    MidiGeneratorParams::GeneratorSettings settings;

    // MIDI generation state
    // Monophonic note tracking
    int currentActiveNote = -1;
    int currentActiveVelocity = 0;
    juce::int64 noteStartTime = 0;
    juce::int64 noteDuration = 0;
    bool noteIsActive = false;

    int currentInputNote = -1;
    int currentInputVelocity = 0;
    bool isInputNoteActive = false;
    int currentActiveSample = -1;

    // Timing state
    double sampleRate = 44100.0;
    juce::int64 samplePosition = 0;
    double bpm = 120.0;
    double ppqPosition = 0.0;
    double lastPpqPosition = 0.0;
    double lastTriggerTimes[MidiGeneratorParams::NUM_RATE_OPTIONS] = {0.0};

    // Sample management state
    bool useRandomSample = false;
    float randomizeProbability = 100.0f; // 0-100%

    // Randomized values for visualization 
    std::atomic<float> currentRandomizedGate{0.0f};
    std::atomic<float> currentRandomizedVelocity{0.0f};

    // Helper methods for MIDI generation
    void updateSettingsFromParameters();
    void stopActiveNote(juce::MidiBuffer& midiMessages, int samplePosition);
    
    // Note timing and generation
    double getNoteDurationInSamples(MidiGeneratorParams::RateOption rate);
    bool shouldTriggerNote(MidiGeneratorParams::RateOption rate);
    int calculateNoteLength(MidiGeneratorParams::RateOption rate);
    int calculateVelocity();
    
    // Note modification
    int applyScaleAndModifications(int noteNumber);
    bool isNoteInScale(int note, juce::Array<int> scale, int root);
    int findClosestNoteInScale(int note, juce::Array<int> scale, int root);
    juce::Array<int> getSelectedScale();
    
    // Helper for randomization
    float applyRandomization(float value, float randomizeValue) const;

    struct EligibleRate
    {
        MidiGeneratorParams::RateOption rate;
        float weight;
    };

    // Helper methods for processBlock
    void updateTimingInfo();
    void processIncomingMidi(const juce::MidiBuffer& midiMessages, juce::MidiBuffer& processedMidi, int numSamples);
    void checkActiveNotes(juce::MidiBuffer& midiMessages, int numSamples);
    std::vector<EligibleRate> collectEligibleRates(float& totalWeight);
    MidiGeneratorParams::RateOption selectRateFromEligible(const std::vector<EligibleRate>& eligibleRates, float totalWeight);
    void generateNewNotes(juce::MidiBuffer& midiMessages);
    void playNewNote(MidiGeneratorParams::RateOption selectedRate, juce::MidiBuffer& midiMessages);
    void processAudio(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& processedMidi, juce::MidiBuffer& midiMessages);


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiGeneratorProcessor)
};
