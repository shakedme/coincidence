#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "Sampler/SampleManager.h"

class PluginProcessor;

/**
 * Class to handle audio processing functionality
 */
class CoincidenceAudioProcessor
{
public:
    CoincidenceAudioProcessor(PluginProcessor& processor);
    ~CoincidenceAudioProcessor() = default;
    
    // Initialize with sample rate
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    
    // Clear state before releasing resources
    void releaseResources();
    
    // Process audio
    void processAudio(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& processedMidi,
                      juce::MidiBuffer& midiMessages);
    
    // Access sample manager
    SampleManager& getSampleManager() { return sampleManager; }
    
private:
    // Reference to the main processor
    PluginProcessor& processor;
    
    // Sample manager
    SampleManager sampleManager;
    
    // Counter for periodic voice cleanup
    int bufferCounter = 0;
}; 