#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../Params.h"
#include "../Shared/TimingManager.h"
#include "../Sampler/SampleManager.h"
#include "BaseEffect.h"
#include <vector>

class Reverb : public BaseEffect
{
public:
    Reverb(std::shared_ptr<TimingManager> t, SampleManager& sm);
    ~Reverb() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void setSettings(Params::FxSettings s) override;
    void applyReverbEffect(juce::AudioBuffer<float>& buffer, 
                          const std::vector<juce::int64>& triggerSamplePositions,
                          const std::vector<juce::int64>& noteDurations);

    bool shouldApplyReverb();

private:
    struct ActiveReverb
    {
        juce::int64 startSample;
        juce::int64 duration;
        juce::int64 currentPosition;
        bool isActive;
    };

    juce::Reverb juceReverb;
    juce::Reverb::Parameters juceReverbParams;

    double sampleRate {44100.0};
    int currentBufferSize {512};
    
    // Track active reverb effects
    ActiveReverb activeReverb;
    
    const double MIN_TIME_BETWEEN_TRIGGERS_SECONDS = 3.0;
    
    // Track the last time the reverb was triggered
    juce::int64 lastTriggerSample = 0;

    bool isReverbEnabledForSample();
    void processActiveReverb(juce::AudioBuffer<float>& buffer, 
                             const juce::AudioBuffer<float>& reverbBuffer, 
                             float wetMix);
    void processNewReverbTrigger(juce::AudioBuffer<float>& buffer,
                                const juce::AudioBuffer<float>& reverbBuffer,
                                const std::vector<juce::int64>& triggerSamplePositions,
                                const std::vector<juce::int64>& noteDurations,
                                float wetMix);
};
