//
// Created by Shaked Melman on 21/03/2025.
//

#include "SamplerVoiceState.h"

SamplerSound *SamplerVoiceState::getCurrentSound() {
    auto it = indexToSoundMap.find(currentSampleIndex);
    if (it != indexToSoundMap.end()) {
        return it->second;
    }

    if (!indexToSoundMap.empty()) {
        return indexToSoundMap.begin()->second;
    }

    return nullptr;
}

void SamplerVoiceState::registerSoundWithIndex(SamplerSound *sound, int index) {
    if (sound != nullptr) {
        indexToSoundMap[index] = sound;
    }
}

void SamplerVoiceState::setADSRParameters(float attackMs, float decayMs, float sustain, float releaseMs) {
    // Convert from milliseconds to seconds for JUCE ADSR
    adsrParams.attack = attackMs / 1000.0f;  // milliseconds to seconds
    adsrParams.decay = decayMs / 1000.0f;    // milliseconds to seconds
    adsrParams.sustain = sustain;            // already correct (0-1)
    adsrParams.release = releaseMs / 1000.0f; // milliseconds to seconds
}
