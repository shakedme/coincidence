//
// Created by Shaked Melman on 21/03/2025.
//

#ifndef COINCIDENCE_SAMPLERSOUND_H
#define COINCIDENCE_SAMPLERSOUND_H

#include <juce_audio_utils/juce_audio_utils.h>

class SamplerSound : public juce::SynthesiserSound {
public:
    SamplerSound(juce::String name,
                 juce::AudioFormatReader &source,
                 juce::BigInteger notes);

    bool appliesToNote(int midiNoteNumber) override;

    bool appliesToChannel(int midiChannel) override;

    juce::AudioBuffer<float> *getAudioData() { return &audioData; }

    double getSourceSampleRate() const { return sourceSampleRate; }

    int getIndex() const { return index; }

    void setIndex(int idx) { index = idx; }

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

    void clearOnsetMarkers() { onsetMarkers.clear(); }

    bool isOnsetRandomizationEnabled() const { return useOnsetRandomization; }

    void setOnsetRandomizationEnabled(bool enabled) { useOnsetRandomization = enabled; }

private:
    juce::String name;
    juce::AudioBuffer<float> audioData;
    juce::BigInteger midiNotes;
    double sourceSampleRate;
    int index = -1; // Sample index
    int groupIndex = -1; // Group index, -1 means no group
    float startMarkerPosition = 0.0f;
    float endMarkerPosition = 1.0f;
    std::vector<float> onsetMarkers; // Onset marker positions (0.0-1.0)
    bool useOnsetRandomization = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerSound)
};


#endif //COINCIDENCE_SAMPLERSOUND_H
