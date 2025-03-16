#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../../Shared/Models.h"
#include "../../Shared/StateManager.h"
#include "../../Shared/TimingManager.h"
#include "../Sampler/SampleManager.h"
#include "BaseEffect.h"
#include <vector>

class Delay : public BaseEffect
{
public:
    Delay(PluginProcessor& processor);
    ~Delay() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void applyDelayEffect(juce::AudioBuffer<float>& buffer,
                          const std::vector<juce::int64>& triggerSamplePositions);
private:
    std::unique_ptr<AppState::ParameterBinding<Models::DelaySettings>> paramBinding;
    Models::DelaySettings settings;

    // Delay line setup
    std::unique_ptr<juce::dsp::DelayLine<float>> delayLineLeft;
    std::unique_ptr<juce::dsp::DelayLine<float>> delayLineRight;
    float delayTimeInSamples {0.0f};

    struct ActiveDelay {
        juce::int64 startSample;
        juce::int64 duration;
        juce::int64 currentPosition;
        bool isActive;
    };
    
    // Track active delay effect
    ActiveDelay activeDelay;
    
    // Helper methods
    bool shouldApplyDelay();
    bool isDelayEnabledForSample();
    float calculateDelayTimeFromBPM(float rate);
    void recalculateDelayTimeInSamples();
    
    // Delay processing methods
    void processPingPongDelay(const juce::AudioBuffer<float> &buffer, 
                             juce::AudioBuffer<float> &delayBuffer,
                             float feedbackAmount, int sample);
    void processStereoDelay(const juce::AudioBuffer<float> &buffer,
                           juce::AudioBuffer<float> &delayBuffer,
                           float feedbackAmount,
                           int channel,
                           int sample);
    void processActiveDelay(juce::AudioBuffer<float>& buffer, 
                           const juce::AudioBuffer<float>& delayBuffer, 
                           float wetMix);
    void processNewDelayTrigger(juce::AudioBuffer<float>& buffer,
                               const juce::AudioBuffer<float>& delayBuffer,
                               const std::vector<juce::int64>& triggerSamplePositions,
                               float wetMix);
};