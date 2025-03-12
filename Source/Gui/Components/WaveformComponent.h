#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <atomic>
#include <mutex>
#include "../../Audio/Util/AudioBufferQueue.h"

/**
 * A dedicated component for visualizing audio waveforms.
 * Uses a lock-free mechanism to receive audio data from the audio thread
 * and display it on the GUI thread without blocking.
 */
class WaveformComponent : public juce::Component, private juce::Timer
{
public:
    WaveformComponent();
    ~WaveformComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Set the sample rate for audio visualization
    void setSampleRate(float newSampleRate);
    
    // Set time range in seconds (for scaling the waveform)
    void setTimeRange(float seconds);
    
    // Set the factor to scale the waveform by
    void setWaveformScaleFactor(float scale);
    
    // Set the color of the waveform
    void setWaveformColour(juce::Colour colour);
    
    // Set the background color (use transparent for layering with other components)
    void setBackgroundColour(juce::Colour colour);
    
    // Set transparency of the waveform (0.0 = invisible, 1.0 = fully opaque)
    void setWaveformAlpha(float alpha);
    
    // Thread-safe method to push audio buffer from audio thread
    void pushAudioBuffer(const float* audioData, int numSamples);
    
private:
    // Initialize the waveform rendering system
    void setupWaveformRendering();
    
    // Update the waveform cache image
    void updateWaveformCache();
    
    // Timer callback to update visualization
    void timerCallback() override;
    
    // Waveform visualization
    AudioBufferQueue* audioBufferQueue = nullptr;
    std::vector<float> waveformData;
    
    struct PeakData {
        float min = 0.0f;
        float max = 0.0f;
    };
    
    std::vector<PeakData> waveformPeaks;
    juce::Image waveformCache;
    std::atomic<bool> waveformNeedsRedraw{false};
    std::mutex waveformMutex;
    
    // Waveform settings
    float sampleRate = 44100.0f;
    float timeRangeInSeconds = 1.0f;
    float waveformScaleFactor = 1.0f;
    float waveformAlpha = 0.5f;
    juce::Colour waveformColour = juce::Colour(0xff52bfd9);
    juce::Colour backgroundColour = juce::Colour(0xff222222);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformComponent)
}; 