#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../Params.h"
#include "../Shared/TimingManager.h"
#include "FDNReverb.h"
#include <vector>

class Reverb
{
public:
    Reverb(std::shared_ptr<TimingManager> t);
    ~Reverb() = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();
    void setSettings(Params::FxSettings s);
    void setBufferSize(int numSamples);
    
    void applyReverbEffect(juce::AudioBuffer<float>& buffer, 
                          const std::vector<juce::int64>& triggerSamplePositions,
                          const std::vector<juce::int64>& noteDurations);

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
    FDNReverb fdnReverb;
    Params::FxSettings settings;
    
    double sampleRate {44100.0};
    int currentBufferSize {512};
    
    // Track active reverb effects
    ActiveReverb activeReverb;
    
    bool shouldApplyReverb();
};
