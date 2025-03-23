#pragma once

#include "SamplerSound.h"
#include "SamplerVoice.h"
#include "SamplerVoiceState.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <utility>
#include <vector>
#include <memory>
#include "../../Shared/Models.h"
#include "OnsetDetector.h"
#include <map>
#include <unordered_map>

class SampleManager : public juce::AudioProcessorValueTreeState::Listener {
public:
    SampleManager(PluginProcessor &processor);

    ~SampleManager();

    struct SampleInfo {
        juce::String name;
        juce::File file;
        int index;
        std::unique_ptr<SamplerSound> sound;
        float probability = 1.0f; // Default probability value (1.0 = 100%)
        int groupIndex = -1;      // -1 means not part of any group, 0-3 for groups

        std::unordered_map<Models::RateOption, bool> rateEnabled;

        SampleInfo(juce::String n, juce::File f, int idx);
    };

    // Group of samples
    struct Group {
        juce::String name;
        int index;
        float probability = 1.0f;
        std::vector<int> sampleIndices;

        std::unordered_map<Models::RateOption, bool> rateEnabled;

        Group(juce::String n, int idx) : name(std::move(n)), index(idx) {
            // Initialize all rates and effects to enabled by default
            for (int i = 0; i < Models::NUM_RATE_OPTIONS; ++i) {
                rateEnabled[static_cast<Models::RateOption>(i)] = true;
            }
        }
    };

    void parameterChanged(const juce::String &parameterID, float newValue) override;

    void processAudio(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &processedMidi);

    // Sample management
    void addSample(const juce::File &file);

    void removeSamples(int startIdx, int endIdx);

    void clearAllSamples();

    void normalizeSamples();

    juce::File getSampleFilePath(int index) const;

    int getNextSampleIndex(Models::RateOption currentRate);

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

    // Voice state control (replacing static methods in SamplerVoice)
    void setCurrentSampleIndex(int sampleIndex) { voiceState.setCurrentSampleIndex(sampleIndex); }

    int getCurrentSampleIndex() const { return voiceState.getCurrentSampleIndex(); }

    void registerSoundWithIndex(SamplerSound *sound, int index) { voiceState.registerSoundWithIndex(sound, index); }

    SamplerSound *getCorrectSoundForIndex(int index) { return voiceState.getCorrectSoundForIndex(index); }

    void clearSoundRegistrations() { voiceState.clearSoundRegistrations(); }

    void setMaxPlayDurationForSample(juce::int64 durationInSamples);

    void prepareToPlay(double sampleRate);

    void setSampleRateEnabled(int sampleIndex, Models::RateOption rate, bool enabled);

    bool isSampleRateEnabled(int sampleIndex, Models::RateOption rate) const;

    void setGroupRateEnabled(int groupIndex, Models::RateOption rate, bool enabled);

    bool isGroupRateEnabled(int groupIndex, Models::RateOption rate) const;

private:
    // Current sample state (for playback)
    int currentSelectedSample = -1;
    int currentPlayIndex = -1; // Tracks the index for sequential/bidirectional playback
    bool isAscending = true;   // For bidirectional mode

    // Loaded samples
    std::vector<std::unique_ptr<SampleInfo>> sampleList;

    // Replace six separate vectors with a single unordered_map
    std::unordered_map<Models::RateOption, std::vector<int>> validSamplesForRate;

    // Helper methods for managing valid sample lists
    void updateValidSamplesForRate(Models::RateOption rate);

    const std::vector<int> &getValidSamplesForRate(Models::RateOption rate) const;

    // Random sample selection helper methods
    int selectRandomSampleWithProbability(const std::vector<int> &validSamples);

    void organizeValidSamplesByGroup(const std::vector<int> &validSamples,
                                     std::map<int, std::vector<int>> &groupedValidSamples,
                                     float &totalGroupProbability);

    int selectFromGroupedSamples(const std::map<int, std::vector<int>> &groupedValidSamples,
                                 float totalGroupProbability);

    int selectGroup(const std::map<int, std::vector<int>> &groupedValidSamples, float totalGroupProbability);

    int selectSampleFromGroup(const std::vector<int> &samplesInGroup);

    PluginProcessor &processor;

    // Sample groups (max 8 groups)
    std::vector<std::unique_ptr<Group>> groups;

    // Playback engine
    juce::Synthesiser sampler;

    // Voice state for SamplerVoice instances
    SamplerVoiceState voiceState;

    // Format manager for loading audio files
    juce::AudioFormatManager formatManager;

    OnsetDetector onsetDetector;

    Models::DirectionType sampleDirection = Models::DirectionType::RANDOM;
};
