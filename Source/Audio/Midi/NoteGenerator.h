#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../Params.h"
#include "../Shared/TimingManager.h"
#include "ScaleManager.h"

class PluginProcessor;
class PluginEditor;

/**
 * Class to handle note generation and MIDI processing functionality
 */
class NoteGenerator
{
public:
    struct EligibleRate
    {
        Params::RateOption rate;
        float weight;
    };
    
    struct PendingNote 
    {
        int noteNumber;
        int velocity;
        juce::int64 startSamplePosition; // Absolute sample position
        juce::int64 durationInSamples;
        int sampleIndex = -1; // For sample playback
    };

    NoteGenerator(PluginProcessor& p, std::shared_ptr<TimingManager> t);
    ~NoteGenerator() = default;

    // Initialize with sample rate
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    
    // Clear state before releasing resources
    void releaseResources();
    
    // Collect all rates that should trigger
    std::vector<EligibleRate> collectEligibleRates(const Params::GeneratorSettings& settings, float& totalWeight);
    
    // Select a rate from eligible rates based on weighted probability
    Params::RateOption selectRateFromEligible(const std::vector<EligibleRate>& eligibleRates, float totalWeight);
    
    // Generate new notes based on settings
    void generateNewNotes(juce::MidiBuffer& midiMessages, const Params::GeneratorSettings& settings);
    
    // Play a new note at the specified rate
    void playNewNote(Params::RateOption selectedRate, juce::MidiBuffer& midiMessages, const Params::GeneratorSettings& settings);
    
    // Process pending notes (scheduled for future buffers)
    void processPendingNotes(juce::MidiBuffer& midiMessages, int numSamples);
    
    // Calculate the note length in samples
    int calculateNoteLength(Params::RateOption rate, const Params::GeneratorSettings& settings);
    
    // Calculate velocity based on settings
    int calculateVelocity(const Params::GeneratorSettings& settings);
    
    // Apply randomization to a value
    float applyRandomization(float value, float randomizeValue, Params::DirectionType direction) const;
    
    // Process MIDI in for note tracking
    void processIncomingMidi(const juce::MidiBuffer& midiMessages, juce::MidiBuffer& processedMidi, int numSamples);
    
    // Check if active notes need to be turned off
    void checkActiveNotes(juce::MidiBuffer& midiMessages, int numSamples);
    
    // Stop an active note
    void stopActiveNote(juce::MidiBuffer& midiMessages, int currentSamplePosition);
    
    // Accessors
    float getCurrentRandomizedGate() const { return currentRandomizedGate; }
    float getCurrentRandomizedVelocity() const { return currentRandomizedVelocity; }
    int getCurrentActiveSampleIdx() const { return currentActiveSampleIdx; }
    bool isNoteActive() const { return noteIsActive; }
    
    // Get the list of pending notes
    const std::vector<PendingNote>& getPendingNotes() const { return pendingNotes; }
    
private:
    // Reference to the main processor
    PluginProcessor& processor;


    // Managers for specific functionality
    ScaleManager scaleManager;
    std::shared_ptr<TimingManager> timingManager;

    // MIDI generation state
    // Monophonic note tracking
    int currentActiveNote = -1;
    int currentActiveVelocity = 0;
    juce::int64 noteStartPosition = 0;
    juce::int64 noteDurationInSamples = 0;
    bool noteIsActive = false;

    int currentInputNote = -1;
    bool isInputNoteActive = false;
    int currentActiveSampleIdx = -1;
    
    // Pending notes for future processing
    std::vector<PendingNote> pendingNotes;
    
    // Randomized values for visualization
    std::atomic<float> currentRandomizedGate {0.0f};
    std::atomic<float> currentRandomizedVelocity {0.0f};
}; 