#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../Params.h"
#include "../Shared/TimingManager.h"
#include <vector>

class Reverb
{
public:
    Reverb(std::shared_ptr<TimingManager> t);
    ~Reverb() = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();
    void setSettings(Params::FxSettings s);
    void applyReverbEffect(juce::AudioBuffer<float>& buffer, 
                          const std::vector<juce::int64>& triggerSamplePositions,
                          const std::vector<juce::int64>& noteDurations);

    bool shouldApplyReverb();
    
    // Check if reverb should be applied for a specific sample
    static bool shouldApplyReverbForSample(int sampleIndex);

private:
    struct ActiveReverb
    {
        juce::int64 startSample;
        juce::int64 duration;
        juce::int64 currentPosition;
        bool isActive;
    };

    std::shared_ptr<TimingManager> timingManager;
    juce::Reverb juceReverb;
    juce::Reverb::Parameters juceReverbParams;
    Params::FxSettings settings;
    
    double sampleRate {44100.0};
    int currentBufferSize {512};
    
    // Track active reverb effects
    ActiveReverb activeReverb;
    
    // Minimum time between reverb triggers (3 seconds)
    const double MIN_TIME_BETWEEN_TRIGGERS_SECONDS = 5.0;
    
    // Track the last time the reverb was triggered
    juce::int64 lastTriggerSample = 0;
};
