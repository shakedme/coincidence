#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <map>

/**
 * SamplerSound - Custom implementation of SynthesiserSound for sample playback
 */
class SamplerSound : public juce::SynthesiserSound {
public:
    SamplerSound(const juce::String &name,
                 juce::AudioFormatReader &source,
                 const juce::BigInteger &notes);


    // SynthesiserSound methods
    bool appliesToNote(int midiNoteNumber) override;

    bool appliesToChannel(int midiChannel) override;

    // Getters for sample data
    juce::AudioBuffer<float> *getAudioData() { return &audioData; }

    double getSourceSampleRate() const { return sourceSampleRate; }

    // Control sound activation
    bool isActive() const { return isAppropriatelyActive; }

    // Get the sample index for identification
    int getIndex() const { return index; }

    void setIndex(int idx) { index = idx; }

    // Get/set group index
    int getGroupIndex() const { return groupIndex; }

    void setGroupIndex(int idx) { groupIndex = idx; }

    float getStartMarkerPosition() const { return startMarkerPosition; }

    float getEndMarkerPosition() const { return endMarkerPosition; }

    void setMarkerPositions(float start, float end) {
        startMarkerPosition = juce::jlimit(0.0f, 0.99f, start);
        endMarkerPosition = juce::jlimit(startMarkerPosition + 0.01f, 1.0f, end);
    }

    const std::vector<float> &getOnsetMarkers() const { return onsetMarkers; }

    void setOnsetMarkers(const std::vector<float> &markers) { onsetMarkers = markers; }

    void addOnsetMarker(float position) { onsetMarkers.push_back(position); }

    void clearOnsetMarkers() { onsetMarkers.clear(); }

    // Onset randomization mode
    bool isOnsetRandomizationEnabled() const { return useOnsetRandomization; }

    void setOnsetRandomizationEnabled(bool enabled) { useOnsetRandomization = enabled; }

    // Reverb control
    bool isReverbEnabled() const { return reverbEnabled; }
    void setReverbEnabled(bool enabled) { reverbEnabled = enabled; }

    // Stutter control
    bool isStutterEnabled() const { return stutterEnabled; }
    void setStutterEnabled(bool enabled) { stutterEnabled = enabled; }

    // Delay control
    bool isDelayEnabled() const { return delayEnabled; }
    void setDelayEnabled(bool enabled) { delayEnabled = enabled; }

private:
    juce::String name;
    juce::AudioBuffer<float> audioData;
    juce::BigInteger midiNotes;
    double sourceSampleRate;
    bool isAppropriatelyActive = true; // By default, all sounds are active
    int index = -1; // Sample index
    int groupIndex = -1; // Group index, -1 means no group
    float startMarkerPosition = 0.0f;
    float endMarkerPosition = 1.0f;
    std::vector<float> onsetMarkers; // Onset marker positions (0.0-1.0)
    bool useOnsetRandomization = false;
    bool reverbEnabled = true;
    bool stutterEnabled = true;
    bool delayEnabled = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerSound)
};

/**
 * SamplerVoice - Custom implementation of SynthesiserVoice for sample playback
 */
class SamplerVoice : public juce::SynthesiserVoice {
public:
    SamplerVoice();

    static void setPitchFollowEnabled(bool enabled) { pitchFollowEnabled = enabled; }

    static bool isPitchFollowEnabled() { return pitchFollowEnabled; }

    bool canPlaySound(juce::SynthesiserSound *sound) override;

    void renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                         int startSample,
                         int numSamples) override;

    void startNote(int midiNoteNumber,
                   float velocity,
                   juce::SynthesiserSound *sound,
                   int currentPitchWheelPosition) override;

    void stopNote(float velocity, bool allowTailOff) override;

    void pitchWheelMoved(int newPitchWheelValue) override;

    void controllerMoved(int controllerNumber, int newControllerValue) override;

    // Reset all voice state
    void reset();

    // Helper method to check if voice is active
    bool isVoiceActive() const override;

    static void setCurrentSampleIndex(int sampleIndex) { currentGlobalSampleIndex = sampleIndex; }

    static int getCurrentSampleIndex() { return currentGlobalSampleIndex; }

    // New method to register a sound with a specific index
    static void registerSoundWithIndex(SamplerSound *sound, int index);

    // Method to get the appropriate SamplerSound for the current index
    static SamplerSound *getCorrectSoundForIndex(int index);

    // Method to clear all sound registrations
    static void clearSoundRegistrations() { indexToSoundMap.clear(); }

private:
    double pitchRatio = 1.0;
    double sourceSamplePosition = 0.0;
    double sourceEndPosition = 0.0;
    float lgain = 0.0f, rgain = 0.0f;
    bool playing = false;
    int currentSampleIndex = -1;
    // Static shared sample index and sound map
    static int currentGlobalSampleIndex;
    static std::map<int, SamplerSound *> indexToSoundMap;
    static bool pitchFollowEnabled;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerVoice)
};
