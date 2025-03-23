//
// Created by Shaked Melman on 21/03/2025.
//

#include "SamplerVoice.h"


SamplerVoice::SamplerVoice() {
    // Initialize state
    reset();
}

void SamplerVoice::reset() {
    playing = false;
    currentSampleIndex = -1;
    sourceSamplePosition = 0.0;
    pitchRatio = 1.0;
    lgain = 0.0f;
    rgain = 0.0f;

    // Reset the counter for maximum playback duration
    sampleCounter = 0;

    // Reset the ADSR envelope
    adsr.reset();
}

bool SamplerVoice::canPlaySound(juce::SynthesiserSound *sound) {
    // First check if it's a SamplerSound
    auto *samplerSound = dynamic_cast<SamplerSound *>(sound);
    if (samplerSound == nullptr || voiceState == nullptr)
        return false;

    // If we have a valid sample index set through controller, use that instead of the voice's sample index
    // This allows controller-based sample switching to override the assigned sound
    int currentGlobalSampleIndex = voiceState->getCurrentSampleIndex();
    if (currentGlobalSampleIndex >= 0) {
        int soundIndex = samplerSound->getIndex();

        // If it matches the global sample index OR if this voice is not currently playing anything,
        // allow it to play this sound
        return soundIndex == currentGlobalSampleIndex || !isVoiceActive();
    }

    // If no specific sample index is set, any sampler sound can be played
    return true;
}

void SamplerVoice::renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                                   int startSample,
                                   int numSamples) {
    if (!playing || !getCurrentlyPlayingSound() || voiceState == nullptr)
        return;

    // Get the sound that the voice was assigned
    auto *assignedSound = dynamic_cast<SamplerSound *>(getCurrentlyPlayingSound().get());

    if (assignedSound == nullptr) {
        // Safety check: if we have no sound but we're still "playing", clear the note
        playing = false;
        clearCurrentNote();
        return;
    }

    // Try to get the correct sound for the current index if it's different from the assigned sound
    SamplerSound *soundToUse = assignedSound;

    // Only try to switch samples if we have a specific index set and it's different from the assigned sound
    int assignedIndex = assignedSound->getIndex();
    if (currentSampleIndex >= 0 && assignedIndex != currentSampleIndex) {
        // Try to find the correct sound by index
        SamplerSound *correctSound = voiceState->getCorrectSoundForIndex(currentSampleIndex);

        if (correctSound != nullptr && correctSound->isActive()) {
            // Use the correct sound's audio data
            soundToUse = correctSound;
        }
    }

    // Get audio data from the sound we decided to use
    auto &data = *soundToUse->getAudioData();
    const int numChannels = data.getNumChannels();
    const int numSourceSamples = data.getNumSamples();

    // Create a temporary buffer for processing the ADSR envelope
    juce::AudioBuffer<float> tempBuffer;
    tempBuffer.setSize(outputBuffer.getNumChannels(), numSamples);
    tempBuffer.clear();

    // For each sample to render
    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex) {
        // Check if we've exceeded maximum play duration
        if (maxPlayDuration > 0 && sampleCounter >= maxPlayDuration) {
            clearCurrentNote();
            playing = false;
            break;
        }

        // Position in the sample data with bounds checking
        const int sourcePos = static_cast<int>(sourceSamplePosition);

        // If we've reached the end of the sample data or the end marker, stop playback
        if (sourcePos >= numSourceSamples - 1 || sourcePos >= sourceEndPosition) {
            clearCurrentNote();
            playing = false;
            break;
        }

        // Calculate the fractional part for interpolation
        auto alpha = static_cast<float>(sourceSamplePosition - sourcePos);

        // For each channel
        for (int channel = 0;
             channel < std::min(numChannels, tempBuffer.getNumChannels());
             ++channel) {
            // Get sample data pointers
            const float *const inBuffer = data.getReadPointer(channel);
            float *const outBuffer =
                    tempBuffer.getWritePointer(channel, sampleIndex);

            // Ensure we don't access memory out of bounds
            if (sourcePos < 0 || sourcePos >= numSourceSamples - 1) {
                continue;
            }

            // Linear interpolation between adjacent samples
            float sample1 = inBuffer[sourcePos];
            float sample2 = inBuffer[sourcePos + 1];
            float interpolatedSample = sample1 + alpha * (sample2 - sample1);

            // Apply gain (ensuring it's properly scaled)
            const float gain = (channel == 0) ? lgain : rgain;
            *outBuffer = interpolatedSample * gain;
        }

        // Move to next sample position
        sourceSamplePosition += pitchRatio;

        // Increment the sample counter for max duration check
        sampleCounter++;
    }

    // Apply ADSR envelope to the temporary buffer
    adsr.applyEnvelopeToBuffer(tempBuffer, 0, numSamples);

    // Mix the processed temporary buffer into the output buffer
    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel) {
        outputBuffer.addFrom(channel, startSample, tempBuffer, channel, 0, numSamples);
    }

    // If the ADSR has finished its release phase, stop the voice
    if (!adsr.isActive()) {
        clearCurrentNote();
        playing = false;
    }
}

void SamplerVoice::startNote(int midiNoteNumber,
                             float velocity,
                             juce::SynthesiserSound *sound,
                             int /*pitchWheelPosition*/) {
    // Reset voice state first
    reset();

    // No voice state, can't proceed
    if (voiceState == nullptr)
        return;

    // Cast to our custom sound class
    if (auto *samplerSound = dynamic_cast<SamplerSound *>(sound)) {
        // If we're not actively playing this sound, return
        if (!samplerSound->isActive())
            return;

        // Restore sample index selection logic
        // First, prioritize any sample index set through the controller
        int currentGlobalSampleIndex = voiceState->getCurrentSampleIndex();
        if (currentGlobalSampleIndex >= 0) {
            currentSampleIndex = currentGlobalSampleIndex;

            // Get the correct sound for this index if it exists
            if (SamplerSound *correctSound = voiceState->getCorrectSoundForIndex(currentSampleIndex)) {
                // If the correct sound exists but isn't the assigned sound,
                // still use the assigned sound's parameters but note the index change
                if (correctSound != samplerSound) {
                    // We'll continue with the assigned sound but use the requested index
                    // This keeps voice allocation stable while enabling sample switching
                    samplerSound = correctSound; // Use the correct sound for this index
                }
            }
        } else {
            // Use the sample's own index as a fallback
            currentSampleIndex = samplerSound->getIndex();
        }

        double midiNoteHz = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        double soundMidiNoteHz = juce::MidiMessage::getMidiNoteInHertz(60); // C4 as reference

        // Apply pitch ratio based on the global setting
        if (voiceState->isPitchFollowEnabled()) {
            // If pitch follow is enabled, pitch-shift the sample based on MIDI note
            pitchRatio = midiNoteHz / soundMidiNoteHz;
        } else {
            // Otherwise, play at original pitch
            pitchRatio = 1.0;
        }

        // Apply sample rate adjustment
        double ratio = pitchRatio * getSampleRate()
                       / samplerSound->getSourceSampleRate();

        // Set up sample playback positions
        int numSamples = samplerSound->getAudioData()->getNumSamples();
        float startMarker = samplerSound->getStartMarkerPosition();
        float endMarker = samplerSound->getEndMarkerPosition();

        sourceSamplePosition = static_cast<double>(numSamples) * startMarker;
        sourceEndPosition = static_cast<double>(numSamples) * endMarker;

        // Apply the new sample rate ratio
        pitchRatio = ratio;

        const float velocityGain = juce::jmap(velocity, 0.0f, 127.0f, 0.1f, 1.0f);
        lgain = velocityGain;
        rgain = velocityGain;
        juce::Logger::writeToLog("Velocity: " + juce::String(velocity));
        juce::Logger::writeToLog("VelocityGain: " + juce::String(velocityGain));

        updateADSRParameters(voiceState->getADSRParameters());
        setMaxPlayDuration(voiceState->getMaxPlayDuration());

        adsr.noteOn();
        playing = true;
    }
}

void SamplerVoice::stopNote(float /*velocity*/, bool allowTailOff) {
    // Trigger the ADSR release phase
    if (allowTailOff) {
        // If we have a maximum play duration and we've used a significant part of it,
        // we need to adjust the release time to ensure we don't exceed the duration
        if (maxPlayDuration > 0 && sampleCounter > 0) {
            juce::int64 remainingSamples = maxPlayDuration - sampleCounter;

            // If we're close to the maximum duration, adjust release time
            if (remainingSamples < getSampleRate()) // Less than 1 second remaining
            {
                // Get the current ADSR parameters
                auto params = adsr.getParameters();

                // Calculate a safe release time (in seconds) based on remaining samples
                float safeReleaseTime = remainingSamples / getSampleRate();

                // If the current release time is longer than what we can afford, reduce it
                if (params.release > safeReleaseTime) {
                    params.release = safeReleaseTime;
                    adsr.setParameters(params);
                }
            }
        }

        adsr.noteOff();

        // Don't clear the note yet - we'll let the ADSR envelope finish its release phase
        // The voice will be stopped in renderNextBlock when adsr.isActive() becomes false
    } else {
        // Immediate note off - no release phase
        playing = false;
        clearCurrentNote();
        reset();
    }
}

void SamplerVoice::pitchWheelMoved(int newPitchWheelValue) {}

void SamplerVoice::controllerMoved(int controllerNumber, int newControllerValue) {
    if (controllerNumber == 32) {
        // Store the sample index for use when playing
        // Make sure the index exists before setting it
        if (voiceState && voiceState->getCorrectSoundForIndex(newControllerValue) != nullptr) {
            // Store the sample index for use when playing
            currentSampleIndex = newControllerValue;

            // Also update the global sample index
            voiceState->setCurrentSampleIndex(newControllerValue);
        }
    }
    // Other controllers can be handled here if needed
}

// New helper method to check if the voice is active
bool SamplerVoice::isVoiceActive() const {
    return playing && getCurrentlyPlayingSound() != nullptr;
}