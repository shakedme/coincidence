#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../Params.h"
#include "../Shared/TimingManager.h"
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
    void setSettings(Params::FxSettings s);
    void setBufferSize(int numSamples) { currentBufferSize = numSamples; }

private:
    // Settings
    Params::FxSettings settings;

    // JUCE's reverb processor
    juce::Reverb juceReverb;
    juce::Reverb::Parameters juceReverbParams;

    // Timing manager
    std::shared_ptr<TimingManager> timingManager;

    // Most recent buffer size for reference
    int currentBufferSize {0};
    double sampleRate {44100.0};

    // Active notes for selective reverb application
    struct ActiveNote
    {
        int startSample;
        int duration;
        bool active;
    };
    std::vector<ActiveNote> activeNotes;

    // Utility methods
    bool shouldApplyReverb();
    void updateActiveNotes(const std::vector<int>& triggerSamplePositions);
};
