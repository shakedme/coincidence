#pragma once

#include "Sampler.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <memory>
#include "../Params.h"

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
    void removeSamples(int startIdx, int endIdx);
    void clearAllSamples();
    juce::File getSampleFilePath(int index) const;
    int getNextSampleIndex(Params::DirectionType direction);
    void rebuildSounds();
    
    // Getters
    size_t getNumSamples() const { return sampleList.size(); }
    juce::String getSampleName(int index) const;
    bool isSampleLoaded() const { return !sampleList.empty(); }
    SamplerSound* getSampleSound(int index) const;

    // Sampler access
    juce::Synthesiser& getSampler() { return sampler; }
    
    // Setup
    void prepareToPlay(double sampleRate);

private:
    // Current sample state (for playback)
    int currentSelectedSample = -1;
    int currentPlayIndex = -1; // Tracks the index for sequential/bidirectional playback
    bool isAscending = true;   // For bidirectional mode

    // Loaded samples
    std::vector<std::unique_ptr<SampleInfo>> sampleList;
    
    // Playback engine
    juce::Synthesiser sampler;
    
    // Format manager for loading audio files
    juce::AudioFormatManager formatManager;
};
