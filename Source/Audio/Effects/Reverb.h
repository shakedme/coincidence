#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../../Shared/Models.h"
#include "../../Shared/StateManager.h"
#include "../../Shared/TimingManager.h"
#include "../Sampler/SampleManager.h"
#include "BaseEffect.h"
#include <vector>

class Reverb : public BaseEffect
{
public:
    Reverb(PluginProcessor& processor);
    ~Reverb() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void applyReverbEffect(juce::AudioBuffer<float>& buffer,
                          const std::vector<juce::int64>& triggerSamplePositions);

    bool shouldApplyReverb();

private:
    std::unique_ptr<AppState::ParameterBinding<Models::ReverbSettings>> paramBinding;
    Models::ReverbSettings settings;

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

    void processActiveReverb(juce::AudioBuffer<float>& buffer,
                             const juce::AudioBuffer<float>& reverbBuffer, 
                             float wetMix);
    void processNewReverbTrigger(juce::AudioBuffer<float>& buffer,
                                const juce::AudioBuffer<float>& reverbBuffer,
                                const std::vector<juce::int64>& triggerSamplePositions,
                                float wetMix);
};
