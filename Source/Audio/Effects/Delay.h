#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../Params.h"
#include "../Shared/TimingManager.h"
#include <vector>

class Delay
{
public:
    Delay(std::shared_ptr<TimingManager> t);
    ~Delay() = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();
    void setSettings(Params::FxSettings s);
    void applyDelayEffect(juce::AudioBuffer<float>& buffer, 
                          const std::vector<juce::int64>& triggerSamplePositions,
                          const std::vector<juce::int64>& noteDurations);

    bool shouldApplyDelay();

private:
    struct ActiveDelay
    {
        juce::int64 startSample;
        juce::int64 duration;
        juce::int64 currentPosition;
        bool isActive;
    };

    std::shared_ptr<TimingManager> timingManager;
    Params::FxSettings settings;
    
    double sampleRate {44100.0};
    int currentBufferSize {512};
    
    // Delay line setup
    std::unique_ptr<juce::dsp::DelayLine<float>> delayLineLeft;
    std::unique_ptr<juce::dsp::DelayLine<float>> delayLineRight;
    float delayTimeInSamples {0.0f};
    
    // Track active delay effects
    ActiveDelay activeDelay;
    
    // Minimum time between delay triggers (3 seconds)
    const double MIN_TIME_BETWEEN_TRIGGERS_SECONDS = 5.0;
    
    // Track the last time the delay was triggered
    juce::int64 lastTriggerSample = 0;
    
    // Helper method to calculate delay time based on BPM
    float calculateDelayTimeFromBPM(float rate);

    void processPingPongDelay(const juce::AudioBuffer<float> &buffer, juce::AudioBuffer<float> &delayBuffer,
                              float feedbackAmount, int sample);
    void processStereoDelay(const juce::AudioBuffer<float> &buffer,
                            juce::AudioBuffer<float> &delayBuffer,
                            float feedbackAmount,
                            int channel,
                            int sample);
};