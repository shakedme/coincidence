#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../../Shared/Config.h"
#include "../../Shared/TimingManager.h"
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
    void setSettings(Config::FxSettings s) override;
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
    ActiveReverb activeReverb;

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
