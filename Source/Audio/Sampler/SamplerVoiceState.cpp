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