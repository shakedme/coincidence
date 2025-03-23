//
// Created by Shaked Melman on 21/03/2025.
//

#include "SamplerVoiceState.h"

SamplerSound *SamplerVoiceState::getCorrectSoundForIndex(int index) {
    // Look up the sound by index in our map
    auto it = indexToSoundMap.find(index);
    if (it != indexToSoundMap.end()) {
        return it->second;
    }

    // If the map is not empty, just use the first available sound as a fallback
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
    adsrParams.attack = attackMs;
    adsrParams.decay = decayMs;
    adsrParams.sustain = sustain;
    adsrParams.release = releaseMs;
}
