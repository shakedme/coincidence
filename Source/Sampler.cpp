#include "Sampler.h"

SamplerSound::SamplerSound(const juce::String& soundName,
                           juce::AudioFormatReader& source,
                           const juce::BigInteger& midiNotes)
    : name(soundName)
    , midiNotes(midiNotes)
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

// SamplerVoice implementation
SamplerVoice::SamplerVoice()
{

}

bool SamplerVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<SamplerSound*>(sound) != nullptr;
}
void SamplerVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                   int startSample,
                                   int numSamples)
{
    if (!playing)
        return;

    auto* sound = static_cast<SamplerSound*>(getCurrentlyPlayingSound().get());

    if (sound == nullptr)
        return;

    auto& data = *sound->getAudioData();
    const int numChannels = data.getNumChannels();
    const int numSourceSamples = data.getNumSamples();

    // For each sample to render
    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        // Position in the sample data with bounds checking
        const int sourcePos = static_cast<int>(sourceSamplePosition);

        // If we've reached the end of the sample data, stop playback
        if (sourcePos >= numSourceSamples - 1)
        {
            clearCurrentNote();
            playing = false;
            break;
        }

        // Calculate the fractional part for interpolation
        float alpha = static_cast<float>(sourceSamplePosition - sourcePos);

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
                             int /*currentPitchWheelPosition*/)
{
    if (auto* samplerSound = dynamic_cast<SamplerSound*>(sound))
    {
        // Calculate the pitch ratio based on the midi note
        double midiNoteHz = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        double soundMidiNoteHz = juce::MidiMessage::getMidiNoteInHertz(60); // C4 as reference
        pitchRatio = midiNoteHz / soundMidiNoteHz;

        // Account for source sample rate difference
        pitchRatio *= getSampleRate() / samplerSound->getSourceSampleRate();

        // Reset the playback position
        sourceSamplePosition = 0.0;

        // Set the output gains based on velocity (0.0-1.0)
        float velocityGain = velocity;
        lgain = velocityGain;
        rgain = velocityGain;

        // Mark as playing
        playing = true;
    }
    else
    {
        jassertfalse; // This should never happen, but just in case
    }
}

void SamplerVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        // If allowing tail off, you might implement a fade out here
        // For simplicity, we'll just stop immediately
        clearCurrentNote();
        playing = false;
    }
    else
    {
        // Stop immediately
        clearCurrentNote();
        playing = false;
    }
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

void SamplerVoice::controllerMoved(int /*controllerNumber*/, int /*newControllerValue*/)
{
    // Handle MIDI controllers like modulation, expression, etc.
    // This is a simple implementation that doesn't do anything
    // You might want to implement this based on your requirements
}