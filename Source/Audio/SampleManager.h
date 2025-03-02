#pragma once

#include "Sampler.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <memory>

class SampleManager
{
public:
    SampleManager();
    ~SampleManager();

    struct SampleInfo
    {
        juce::String name;
        juce::File file;
        int index;
        std::unique_ptr<SamplerSound> sound;

        SampleInfo(const juce::String& n, const juce::File& f, int idx);
    };

    // Sample management
    void addSample(const juce::File& file);
    void removeSample(int index);
    void clearAllSamples();
    void selectSample(int index);
    int getNextSampleIndex(bool useRandomSample, float randomizeProbability);
    
    // Getters
    int getNumSamples() const { return sampleList.size(); }
    juce::String getSampleName(int index) const;
    bool isSampleLoaded() const { return !sampleList.empty(); }
    int getCurrentSelectedSample() const { return currentSelectedSample; }
    
    // Sampler access
    juce::Synthesiser& getSampler() { return sampler; }
    
    // Setup
    void prepareToPlay(double sampleRate);

private:
    juce::Synthesiser sampler;
    juce::AudioFormatManager formatManager;
    std::vector<std::unique_ptr<SampleInfo>> sampleList;
    int currentSelectedSample = -1;
};
