#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../Shared/Models.h"
#include "../Shared/StateManager.h"
#include "../Shared/TimingManager.h"
#include "Midi/ScaleManager.h"
#include "Midi/NoteGenerator.h"
#include "Sampler/Sampler.h"
#include "Envelope/EnvelopeParameterMapper.h"
#include "Sampler/SampleManager.h"

// Forward declarations
class PluginEditor;

class FxEngine;

class EnvelopeComponent;

/**
 * Main processor class for the MIDI generator plugin.
 * Delegates specific functionality to specialized classes.
 */
class PluginProcessor
        : public juce::AudioProcessor {
public:
    //==============================================================================
    PluginProcessor();

    ~PluginProcessor() override;

    //==============================================================================
    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;

    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;

    bool producesMidi() const override;

    bool isMidiEffect() const override;

    double getTailLengthSeconds() const override;

    int getNumPrograms() override;

    int getCurrentProgram() override;

    void setCurrentProgram(int index) override;

    const juce::String getProgramName(int index) override;

    void changeProgramName(int index, const juce::String &newName) override;

    void getStateInformation(juce::MemoryBlock &destData) override;

    void setStateInformation(const void *data, int sizeInBytes) override;

    // MIDI note state tracking
    bool isNoteActive() const { return noteGenerator->isNoteActive(); }


    // Get current settings
    const Config::MidiSettings &getSettings() const { return settings; }

    SampleManager &getSampleManager() const;

    // Get sample direction type for sample selection
    Config::DirectionType getSampleDirectionType() const;

    // Access to the note generator
    NoteGenerator &getNoteGenerator() const { return *noteGenerator; }

    // Access to the timing manager
    TimingManager &getTimingManager() const { return *timingManager; }

    // Current state values for UI visualization
    float getCurrentRandomizedGate() const { return noteGenerator->getCurrentRandomizedGate(); }

    float getCurrentRandomizedVelocity() const { return noteGenerator->getCurrentRandomizedVelocity(); }

    void updateFxSettingsFromParameters();

    const Config::FxSettings &getGlitchSettings() const { return fxSettings; }

    // Connect the envelope component to receive waveform data
    void connectEnvelopeComponent(EnvelopeComponent *component);

    // Envelope component management
    void setEnvelopeComponent(EnvelopeComponent *component);

    EnvelopeComponent *getEnvelopeComponent() const { return envelopeComponent; }


    // Parameters
    juce::AudioProcessorValueTreeState parameters;
private:
    // Update settings from parameters
    void updateMidiSettingsFromParameters();

    // Plugin state
    Config::MidiSettings settings;
    Config::FxSettings fxSettings;

    AppState::StateManager stateManager;

    // Specialized components for handling different aspects of the plugin
    std::unique_ptr<NoteGenerator> noteGenerator;
    std::unique_ptr<SampleManager> sampleManager;
    std::unique_ptr<FxEngine> fxEngine;
    std::shared_ptr<TimingManager> timingManager;

    // Envelope parameter mapper for amplitude control
    EnvelopeParameterMapper amplitudeEnvelope;

    // Pointer to the envelope component for waveform visualization
    EnvelopeComponent *envelopeComponent = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};

// For debug logging if needed
class FileLogger : public juce::Logger {
public:
    FileLogger();

    void logMessage(const juce::String &message) override;

private:
    juce::File logFile;
};
