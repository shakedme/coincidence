#include "Sampler.h"

#include <utility>

SamplerSound::SamplerSound(juce::String  soundName,
                           juce::AudioFormatReader& source,
                           juce::BigInteger  midiNotes)
    : name(std::move(soundName))
    , midiNotes(std::move(midiNotes))
    , sourceSampleRate(source.sampleRate)
{
    if (source.numChannels > 0)
    {
        audioData.setSize(source.numChannels, static_cast<int>(source.lengthInSamples));

        // Read the entire file into memory
        source.read(
            &audioData, 0, static_cast<int>(source.lengthInSamples), 0, true, true);
    }
}

bool SamplerSound::appliesToNote(int midiNoteNumber)
{
    return midiNotes[midiNoteNumber];
}

bool SamplerSound::appliesToChannel(int /*midiChannel*/)
{
    return true;
}

//==============================================================================
// SamplerVoice Implementation
//==============================================================================

SamplerVoice::SamplerVoice()
{
    // Initialize state
    reset();
}

void SamplerVoice::reset()
{
    playing = false;
    currentSampleIndex = -1;
    sourceSamplePosition = 0.0;
    pitchRatio = 1.0;
    lgain = 0.0f;
    rgain = 0.0f;
}

bool SamplerVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    // First check if it's a SamplerSound
    auto* samplerSound = dynamic_cast<SamplerSound*>(sound);
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

// New helper method to check if the voice is active
bool SamplerVoice::isVoiceActive() const
{
    return playing && getCurrentlyPlayingSound() != nullptr;
}

void SamplerVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                   int startSample,
                                   int numSamples)
{
    if (!playing || !getCurrentlyPlayingSound() || voiceState == nullptr)
        return;

    // Get the sound that the voice was assigned
    auto* assignedSound = static_cast<SamplerSound*>(getCurrentlyPlayingSound().get());

    if (assignedSound == nullptr)
    {
        // Safety check: if we have no sound but we're still "playing", clear the note
        playing = false;
        clearCurrentNote();
        return;
    }

    // Try to get the correct sound for the current index if it's different from the assigned sound
    SamplerSound* soundToUse = assignedSound;

    // Only try to switch samples if we have a specific index set and it's different from the assigned sound
    int assignedIndex = assignedSound->getIndex();
    if (currentSampleIndex >= 0 && assignedIndex != currentSampleIndex)
    {
        // Try to find the correct sound by index
        SamplerSound* correctSound = voiceState->getCorrectSoundForIndex(currentSampleIndex);

        if (correctSound != nullptr && correctSound->isActive())
        {
            // Use the correct sound's audio data
            soundToUse = correctSound;
        }
    }

    // Get audio data from the sound we decided to use
    auto& data = *soundToUse->getAudioData();
    const int numChannels = data.getNumChannels();
    const int numSourceSamples = data.getNumSamples();

    // For each sample to render
    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        // Position in the sample data with bounds checking
        const int sourcePos = static_cast<int>(sourceSamplePosition);

        // If we've reached the end of the sample data or the end marker, stop playback
        if (sourcePos >= numSourceSamples - 1 || sourcePos >= sourceEndPosition)
        {
            clearCurrentNote();
            playing = false;
            break;
        }

        // Calculate the fractional part for interpolation
        auto alpha = static_cast<float>(sourceSamplePosition - sourcePos);

        // For each channel
        for (int channel = 0;
             channel < std::min(numChannels, outputBuffer.getNumChannels());
             ++channel)
        {
            // Get sample data pointers
            const float* const inBuffer = data.getReadPointer(channel);
            float* const outBuffer =
                outputBuffer.getWritePointer(channel, startSample + sampleIndex);

            // Ensure we don't access memory out of bounds
            if (sourcePos < 0 || sourcePos >= numSourceSamples - 1)
            {
                continue;
            }

            // Linear interpolation between adjacent samples
            float sample1 = inBuffer[sourcePos];
            float sample2 = inBuffer[sourcePos + 1];
            float interpolatedSample = sample1 + alpha * (sample2 - sample1);

            // Apply gain (ensuring it's properly scaled)
            const float gain = (channel == 0) ? lgain : rgain;
            *outBuffer += interpolatedSample * gain;
        }

        // Move to next sample position
        sourceSamplePosition += pitchRatio;
    }
}

void SamplerVoice::startNote(int midiNoteNumber,
                            float velocity,
                            juce::SynthesiserSound* sound,
                            int /*pitchWheelPosition*/)
{
    // Reset voice state first
    reset();

    // No voice state, can't proceed
    if (voiceState == nullptr)
        return;

    // Cast to our custom sound class
    if (auto* samplerSound = dynamic_cast<SamplerSound*>(sound))
    {
        // If we're not actively playing this sound, return
        if (!samplerSound->isActive())
            return;

        // Restore sample index selection logic
        // First, prioritize any sample index set through the controller
        int currentGlobalSampleIndex = voiceState->getCurrentSampleIndex();
        if (currentGlobalSampleIndex >= 0) {
            currentSampleIndex = currentGlobalSampleIndex;

            // Get the correct sound for this index if it exists
            if (SamplerSound* correctSound = voiceState->getCorrectSoundForIndex(currentSampleIndex)) {
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
            pitchRatio = midiNoteHz / soundMidiNoteHz;
        } else {
            pitchRatio = 1.0; // Original pitch
        }

        // Account for source sample rate difference
        pitchRatio *= getSampleRate() / samplerSound->getSourceSampleRate();

        // Apply start marker position to calculate starting sample position
        auto& data = *samplerSound->getAudioData();
        int numSamples = data.getNumSamples();

        if (samplerSound->isOnsetRandomizationEnabled()) {
            // Get onset markers
            const auto& onsetMarkers = samplerSound->getOnsetMarkers();

            // If we have onset markers, use them
            if (!onsetMarkers.empty()) {
                // Random index into onset markers array
                int randomIndex = juce::Random::getSystemRandom().nextInt(onsetMarkers.size());

                // Use the random onset position
                sourceSamplePosition = numSamples * onsetMarkers[randomIndex];
            } else {
                // Fallback to start marker if no onset markers
                sourceSamplePosition = numSamples * samplerSound->getStartMarkerPosition();
            }
        } else {
            // Normal playback - use start marker
            sourceSamplePosition = numSamples * samplerSound->getStartMarkerPosition();
        }

        sourceEndPosition = numSamples * samplerSound->getEndMarkerPosition();
            
        // Set the output gains based on velocity (0.0-1.0)
        // MIDI velocity is 0-127, so if velocity is already in that range, don't divide
        float velocityGain = (velocity <= 1.0f) ? velocity : (velocity / 127.0f);
        lgain = velocityGain;
        rgain = velocityGain;
            
        // Flag that we're now playing
        playing = true;
    }
}

void SamplerVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    // If allowTailOff is true, we should allow the note to fade out naturally
    // But for now, we're just stopping immediately regardless
    playing = false;
    
    // Clear the current note to release this voice back to the pool
    clearCurrentNote();
    
    // Reset all voice state
    reset();
}

void SamplerVoice::pitchWheelMoved(int newPitchWheelValue)
{
    // The pitch wheel can adjust the pitch ratio
    // Standard MIDI pitch wheel range is 0-16383, with 8192 as the center (no pitch change)
    // This allows for +/- 2 semitones of pitch bend by default

    // Calculate the pitch bend amount (-1.0 to +1.0)
    float pitchBendAmount = (newPitchWheelValue - 8192) / 8192.0f;

    // Calculate the pitch multiplier (2^(pitchBendAmount/12) for +/- 2 semitones)
    float pitchMultiplier = std::pow(2.0f, pitchBendAmount / 6.0f); // +/- 2 semitones

    // Update the pitch ratio (the base ratio was set in startNote)
    // This assumes you store the base pitch ratio somewhere, which isn't shown in the code
    // So this is just a placeholder, and you might need to adapt it to your design
    // pitchRatio = basePitchRatio * pitchMultiplier;
}

void SamplerVoice::controllerMoved(int controllerNumber, int newControllerValue)
{
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