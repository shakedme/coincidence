//
// Created by Shaked Melman on 21/03/2025.
//

#include "SamplerSound.h"

SamplerSound::SamplerSound(juce::String soundName,
                           juce::AudioFormatReader &source,
                           juce::BigInteger midiNotes)
        : name(std::move(soundName)), midiNotes(std::move(midiNotes)), sourceSampleRate(source.sampleRate) {
    if (source.numChannels > 0) {
        audioData.setSize(source.numChannels, static_cast<int>(source.lengthInSamples));
        source.read(&audioData,
                    0,
                    static_cast<int>(source.lengthInSamples),
                    0,
                    true,
                    true
        );
    }
}

bool SamplerSound::appliesToNote(int midiNoteNumber) {
    return midiNotes[midiNoteNumber];
}

bool SamplerSound::appliesToChannel(int /*midiChannel*/) {
    return true;
}