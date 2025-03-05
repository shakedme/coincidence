#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../Params.h"
#include "../Shared/TimingManager.h"
#include <array>
#include <vector>

class Reverb
{
public:
    Reverb(std::shared_ptr<TimingManager> timingManager);
    ~Reverb() = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();
    void applyReverbEffect(juce::AudioBuffer<float>& buffer, 
                          const std::vector<int>& triggerSamplePositions);

    // Setters
    void setSettings(Params::FxSettings s) { settings = s; }
    void setBufferSize(int numSamples) { currentBufferSize = numSamples; }

private:
    // Settings
    Params::FxSettings settings;
    
    // Timing manager
    std::shared_ptr<TimingManager> timingManager;
    
    // Most recent buffer size for reference
    int currentBufferSize {0};
    double sampleRate {44100.0};
    
    // Constants for the FDN reverb
    static constexpr int NUM_DELAY_LINES = 8;
    static constexpr float MAX_REVERB_TIME = 5.0f; // seconds
    
    // FDN reverb components
    std::array<std::vector<float>, NUM_DELAY_LINES> delayLines;
    std::array<int, NUM_DELAY_LINES> delayLengths;
    std::array<int, NUM_DELAY_LINES> writePositions;
    juce::dsp::Matrix<float> feedbackMatrix;
    
    // Filters for each delay line
    std::array<juce::dsp::IIR::Filter<float>, NUM_DELAY_LINES> lowpassFilters;
    
    // Buffer for processing
    juce::AudioBuffer<float> workBuffer;
    
    // Active notes for selective reverb application
    struct ActiveNote {
        int startSample;
        int duration;
        bool active;
    };
    std::vector<ActiveNote> activeNotes;
    
    // Utility methods
    void initializeFDN();
    void processFDN(juce::AudioBuffer<float>& buffer);
    void applySelectiveReverb(juce::AudioBuffer<float>& buffer, 
                             const std::vector<int>& triggerSamplePositions);
    bool shouldApplyReverb();
    void updateActiveNotes(const std::vector<int>& triggerSamplePositions);
    void clearDelayLines();
};
