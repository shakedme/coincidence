//
// Created by Shaked Melman on 21/03/2025.
//

#ifndef COINCIDENCE_SAMPLERVOICESTATE_H
#define COINCIDENCE_SAMPLERVOICESTATE_H

#include <juce_audio_utils/juce_audio_utils.h>
#include "SamplerSound.h"

class SamplerVoiceState {
public:
    SamplerVoiceState() : currentSampleIndex(-1), pitchFollowEnabled(false) {}

    void setCurrentSampleIndex(int sampleIndex) { currentSampleIndex = sampleIndex; }

    [[nodiscard]] int getCurrentSampleIndex() const { return currentSampleIndex; }

    void registerSoundWithIndex(SamplerSound *sound, int index);

    SamplerSound *getCurrentSound();

    void clearSoundRegistrations() { indexToSoundMap.clear(); }

    [[nodiscard]] bool isPitchFollowEnabled() const { return pitchFollowEnabled; }

    void setPitchFollowEnabled(bool enabled) { pitchFollowEnabled = enabled; }

    void setADSRParameters(float attackMs, float decayMs, float sustain, float releaseMs);

    [[nodiscard]] const juce::ADSR::Parameters &getADSRParameters() const { return adsrParams; }

private:
    int currentSampleIndex;
    std::map<int, SamplerSound *> indexToSoundMap;
    bool pitchFollowEnabled;

    juce::ADSR::Parameters adsrParams{
            0.1f,   // attack (seconds)
            0.1f,   // decay (seconds)
            1.0f,   // sustain level (0-1)
            0.1f    // release (seconds)
    };
};


#endif //COINCIDENCE_SAMPLERVOICESTATE_H
