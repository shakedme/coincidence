#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "Params.h"
#include "Shared/TimingManager.h"
#include "Midi/ScaleManager.h"
#include "Midi/NoteGenerator.h"
#include "JammerAudioProcessor.h"
#include "Effects/FxEngine.h"
#include "Sampler/Sampler.h"

// Forward declarations
class PluginEditor;

/**
 * Main processor class for the MIDI generator plugin.
 * Delegates specific functionality to specialized classes.
 */
class PluginProcessor
    : public juce::AudioProcessor
    , private juce::Timer
{
public:
    //==============================================================================
    PluginProcessor();
    ~PluginProcessor() override;

    //==============================================================================
    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
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
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Timer callback for any background tasks
    void timerCallback() override;
    
    // MIDI note state tracking
    bool isNoteActive() const { return noteGenerator->isNoteActive(); }

    // Parameters
    juce::AudioProcessorValueTreeState parameters;
    
    // Get current settings
    const Params::GeneratorSettings& getSettings() const { return settings; }

    SampleManager& getSampleManager() const;
    // Get sample direction type for sample selection
    Params::DirectionType getSampleDirectionType() const;
    
    // Access to the note generator
    NoteGenerator& getNoteGenerator() const { return *noteGenerator; }
    
    // Current state values for UI visualization
    float getCurrentRandomizedGate() const { return noteGenerator->getCurrentRandomizedGate(); }
    float getCurrentRandomizedVelocity() const { return noteGenerator->getCurrentRandomizedVelocity(); }

    void updateFxSettingsFromParameters();
    const Params::FxSettings& getGlitchSettings() const { return fxSettings; }
private:
    // Update settings from parameters
    void updateMidiSettingsFromParameters();
    
    // Plugin state
    Params::GeneratorSettings settings;
    Params::FxSettings fxSettings;

    // Specialized components for handling different aspects of the plugin
    std::unique_ptr<NoteGenerator> noteGenerator;
    std::unique_ptr<JammerAudioProcessor> audioProcessor;
    std::unique_ptr<FxEngine> fxEngine;
    std::shared_ptr<TimingManager> timingManager;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};

// For debug logging if needed
class FileLogger : public juce::Logger
{
public:
    FileLogger();
    void logMessage(const juce::String& message) override;

private:
    juce::File logFile;
};
