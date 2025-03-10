#pragma once

#include "Sampler.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <memory>
#include "../Params.h"
#include "OnsetDetector.h"
#include <map>

class SampleManager {
public:
    SampleManager();

    ~SampleManager();

    struct SampleInfo {
        juce::String name;
        juce::File file;
        int index;
        std::unique_ptr<SamplerSound> sound;
        float probability = 1.0f; // Default probability value (1.0 = 100%)
        int groupIndex = -1;      // -1 means not part of any group, 0-3 for groups

        // Rate flags - all enabled by default
        bool rate_1_1_enabled = true;
        bool rate_1_2_enabled = true;
        bool rate_1_4_enabled = true;
        bool rate_1_8_enabled = true;
        bool rate_1_16_enabled = true;
        bool rate_1_32_enabled = true;

        SampleInfo(const juce::String &n, const juce::File &f, int idx);
    };

    // Group of samples
    struct Group {
        juce::String name;
        int index;
        float probability = 1.0f; // Default probability value (1.0 = 100%)
        std::vector<int> sampleIndices;

        // Rate flags - all enabled by default
        bool rate_1_1_enabled = true;
        bool rate_1_2_enabled = true;
        bool rate_1_4_enabled = true;
        bool rate_1_8_enabled = true;
        bool rate_1_16_enabled = true;
        bool rate_1_32_enabled = true;

        // Effect flags - all enabled by default
        bool reverb_enabled = true;
        bool stutter_enabled = true;
        bool delay_enabled = true;

        Group(const juce::String &n, int idx) : name(n), index(idx) {}
    };

    // Sample management
    void addSample(const juce::File &file);

    void removeSamples(int startIdx, int endIdx);

    void clearAllSamples();

    void normalizeSamples();

    juce::File getSampleFilePath(int index) const;

    int getNextSampleIndex(Params::DirectionType direction, Params::RateOption currentRate);

    void rebuildSounds();

    void createGroup(const juce::Array<int> &sampleIndices);

    void removeGroup(int groupIndex);

    void removeSampleFromGroup(int sampleIndex);

    int getNumGroups() const { return static_cast<int>(groups.size()); }

    const Group *getGroup(int index) const;

    void setSampleProbability(int sampleIndex, float probability);

    float getSampleProbability(int sampleIndex) const;

    void setGroupProbability(int groupIndex, float probability);

    float getGroupProbability(int groupIndex) const;

    // Getters
    size_t getNumSamples() const { return sampleList.size(); }

    juce::String getSampleName(int index) const;

    bool isSampleLoaded() const { return !sampleList.empty(); }

    SamplerSound *getSampleSound(int index) const;

    // Sampler access
    juce::Synthesiser &getSampler() { return sampler; }

    // Setup
    void prepareToPlay(double sampleRate);

    // Sample rate methods
    void setSampleRateEnabled(int sampleIndex, Params::RateOption rate, bool enabled);

    bool isSampleRateEnabled(int sampleIndex, Params::RateOption rate) const;

    // Group rate methods
    void setGroupRateEnabled(int groupIndex, Params::RateOption rate, bool enabled);

    bool isGroupRateEnabled(int groupIndex, Params::RateOption rate) const;

    // Group effect methods (0=reverb, 1=stutter, 2=delay)
    void setGroupEffectEnabled(int groupIndex, int effectType, bool enabled);

    bool isGroupEffectEnabled(int groupIndex, int effectType) const;

private:
    // Current sample state (for playback)
    int currentSelectedSample = -1;
    int currentPlayIndex = -1; // Tracks the index for sequential/bidirectional playback
    bool isAscending = true;   // For bidirectional mode

    // Loaded samples
    std::vector<std::unique_ptr<SampleInfo>> sampleList;

    // Pre-filtered lists of valid samples for each rate
    std::vector<int> validSamples_1_1;
    std::vector<int> validSamples_1_2;
    std::vector<int> validSamples_1_4;
    std::vector<int> validSamples_1_8;
    std::vector<int> validSamples_1_16;
    std::vector<int> validSamples_1_32;

    // Helper methods for managing valid sample lists
    void updateValidSamplesForRate(Params::RateOption rate);

    const std::vector<int> &getValidSamplesForRate(Params::RateOption rate) const;

    // Random sample selection helper methods
    int selectRandomSampleWithProbability(const std::vector<int> &validSamples);

    void organizeValidSamplesByGroup(const std::vector<int> &validSamples,
                                     std::map<int, std::vector<int>> &groupedValidSamples,
                                     float &totalGroupProbability);

    int selectFromGroupedSamples(const std::map<int, std::vector<int>> &groupedValidSamples,
                                 float totalGroupProbability,
                                 const std::vector<int> &validSamples);

    int selectGroup(const std::map<int, std::vector<int>> &groupedValidSamples, float totalGroupProbability);

    int selectSampleFromGroup(const std::vector<int> &samplesInGroup);

    // Sample groups (max 8 groups)
    std::vector<std::unique_ptr<Group>> groups;

    // Playback engine
    juce::Synthesiser sampler;

    // Format manager for loading audio files
    juce::AudioFormatManager formatManager;

    OnsetDetector onsetDetector;
};
