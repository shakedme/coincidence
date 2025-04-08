#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../../Shared/Models.h"
#include "../../Shared/TimingManager.h"
#include "../../Shared/Parameters/StructParameter.h"
#include "../../Shared/Parameters/Params.h"
#include "ScaleManager.h"

class PluginProcessor;
class ScaleManager;
class PluginEditor;

/**
 * Class to handle note generation and MIDI processing functionality
 */
class NoteGenerator {
public:
    struct EligibleRate {
        Models::RateOption rate;
        float weight;
    };

    struct PendingNote {
        int noteNumber;
        int velocity;
        juce::int64 startSamplePosition; // Absolute sample position
        juce::int64 durationInSamples;
        int sampleIndex = -1; // For sample playback
    };

    NoteGenerator(PluginProcessor &p);

    ~NoteGenerator() = default;

    // Initialize with sample rate
    void prepareToPlay(double sampleRate, int samplesPerBlock);

    // Clear state before releasing resources
    void releaseResources();

    // Process MIDI in for note tracking
    void processIncomingMidi(const juce::MidiBuffer &midiMessages,
                             juce::MidiBuffer &processedMidi,
                             int numSamples);

    // Accessors
    float getCurrentRandomizedGate() const { return currentRandomizedGate; }

    float getCurrentRandomizedVelocity() const { return currentRandomizedVelocity; }

    int getCurrentActiveSampleIdx() const { return currentActiveSampleIdx; }

    bool isNoteActive() const { return noteIsActive; }

    juce::int64 getCurrentNoteDuration() const { return noteDurationInSamples; }

    // Get the list of pending notes
    const std::vector<PendingNote> &getPendingNotes() const { return pendingNotes; }

private:
    std::unique_ptr<StructParameter<Models::MidiSettings>> settingsBinding;
    Models::MidiSettings settings;

    // Reference to the main processor
    PluginProcessor &processor;


    // Managers for specific functionality
    std::unique_ptr<ScaleManager> scaleManager;
    TimingManager &timingManager;

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
    std::atomic<float> currentRandomizedGate{0.0f};
    std::atomic<float> currentRandomizedVelocity{0.0f};

    void addNoteWithinCurrentBuffer(juce::MidiBuffer &buffer,
                                    int play,
                                    int velocity,
                                    int offset,
                                    juce::int64 position,
                                    int samples,
                                    int index);

    void addPendingNote(int noteToPlay,
                        int velocity,
                        int noteLengthSamples,
                        int sampleIndex,
                        juce::int64 position);

    // Check if active notes need to be turned off
    void checkActiveNotes(juce::MidiBuffer &midiMessages, int numSamples);

    // Generate new notes based on settings
    void generateNewNotes(juce::MidiBuffer &midiMessages);

    // Collect all rates that should trigger
    std::vector<EligibleRate> collectEligibleRates(float &totalWeight);

    // Select a rate from eligible rates based on weighted probability
    Models::RateOption selectRateFromEligible(const std::vector<EligibleRate> &eligibleRates, float totalWeight);

    // Play a new note at the specified rate
    void playNewNote(Models::RateOption selectedRate, juce::MidiBuffer &midiMessages);

    // Process pending notes (scheduled for future buffers)
    void processPendingNotes(juce::MidiBuffer &midiMessages, int numSamples);

    // Calculate the note length in samples
    int calculateNoteLength(Models::RateOption rate);

    // Calculate velocity based on settings
    int calculateVelocity();

    // Apply randomization to a value
    float applyRandomization(float value, float randomizeValue, Models::DirectionType direction) const;

    // Stop an active note
    void stopActiveNote(juce::MidiBuffer &midiMessages, int currentSamplePosition);

};