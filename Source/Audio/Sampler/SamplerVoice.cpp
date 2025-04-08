//
// Created by Shaked Melman on 21/03/2025.
//

#include "SamplerVoice.h"


SamplerVoice::SamplerVoice(SamplerVoiceState &state)
        : voiceState(state) {
    reset();
}

void SamplerVoice::reset() {
    playing = false;
    currentSampleIndex = -1;
    sourceSamplePosition = 0.0;
    pitchRatio = 1.0;
    lgain = 0.0f;
    rgain = 0.0f;
}

bool SamplerVoice::canPlaySound(juce::SynthesiserSound *sound) {
    return dynamic_cast<SamplerSound *>(sound);
}

void SamplerVoice::renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                                   int startSample,
                                   int numSamples) {
    if (!playing) {
        return;
    }

    auto *soundToUse = voiceState.getCurrentSound();
    if (soundToUse == nullptr) {
        return;
    }

    auto &data = *soundToUse->getAudioData();
    const int numChannels = data.getNumChannels();
    const int numSourceSamples = data.getNumSamples();

    juce::AudioBuffer<float> tempBuffer;
    tempBuffer.setSize(outputBuffer.getNumChannels(), numSamples);
    tempBuffer.clear();

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex) {
        const int sourcePos = static_cast<int>(sourceSamplePosition);

        if (sourcePos >= numSourceSamples - 1 || sourcePos >= sourceEndPosition || sourcePos < 0) {
            clearCurrentNote();
            playing = false;
            break;
        }

        auto alpha = static_cast<float>(sourceSamplePosition - sourcePos);

        const int minChannels = std::min(numChannels, tempBuffer.getNumChannels());
        for (int channel = 0; channel < minChannels; ++channel) {
            const float *const inBuffer = data.getReadPointer(channel);
            float *const outBuffer = tempBuffer.getWritePointer(channel, sampleIndex);

            // linear interpolation for pitch shifting
            float sample1 = inBuffer[sourcePos];
            float sample2 = inBuffer[sourcePos + 1];
            float interpolatedSample = sample1 + alpha * (sample2 - sample1);

            const float gain = (channel == 0) ? lgain : rgain;
            *outBuffer = interpolatedSample * gain;
        }

        sourceSamplePosition += pitchRatio;
    }

    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel) {
        outputBuffer.addFrom(channel, startSample, tempBuffer, channel, 0, numSamples);
    }
}

void SamplerVoice::startNote(int midiNoteNumber,
                             float velocity,
                             juce::SynthesiserSound *sound,
                             int /*pitchWheelPosition*/) {
    reset();

    if (auto *samplerSound = dynamic_cast<SamplerSound *>(sound)) {
        double midiNoteHz = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        double soundMidiNoteHz = juce::MidiMessage::getMidiNoteInHertz(60);

        if (voiceState.isPitchFollowEnabled()) {
            pitchRatio = midiNoteHz / soundMidiNoteHz;
        } else {
            pitchRatio = 1.0;
        }

        double ratio = pitchRatio * getSampleRate()
                       / samplerSound->getSourceSampleRate();

        int numSamples = samplerSound->getAudioData()->getNumSamples();
        float startMarker = samplerSound->getStartMarkerPosition();
        float endMarker = samplerSound->getEndMarkerPosition();

        sourceSamplePosition = static_cast<double>(numSamples) * startMarker;
        sourceEndPosition = static_cast<double>(numSamples) * endMarker;
        pitchRatio = ratio;
        lgain = velocity;
        rgain = velocity;
        playing = true;
    }
}

void SamplerVoice::stopNote(float /*velocity*/, bool allowTailOff) {
    clearCurrentNote();
    playing = false;
    reset();
}

void SamplerVoice::pitchWheelMoved(int newPitchWheelValue) {}

void SamplerVoice::controllerMoved(int controllerNumber, int newControllerValue) {}

bool SamplerVoice::isVoiceActive() const {
    return playing && getCurrentlyPlayingSound() != nullptr;
}