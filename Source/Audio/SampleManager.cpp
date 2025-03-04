#include "SampleManager.h"

SampleManager::SampleInfo::SampleInfo(const juce::String& n, const juce::File& f, int idx)
    : name(n)
    , file(f)
    , index(idx)
{
}

SampleManager::SampleManager()
{
    // Initialize format manager
    formatManager.registerBasicFormats();

    // Set up synth voices - increase from 32 to 64 voices for more polyphony
    for (int i = 0; i < 64; ++i)
        sampler.addVoice(new SamplerVoice());

    // Enable voice stealing to handle when all voices are in use
    sampler.setNoteStealingEnabled(true);
}

SampleManager::~SampleManager()
{
    clearAllSamples();
}

void SampleManager::prepareToPlay(double sampleRate)
{
    sampler.setCurrentPlaybackSampleRate(sampleRate);

    // Reset all voices to ensure they're in a clean state
    sampler.allNotesOff(0, true);
}

void SampleManager::addSample(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (reader != nullptr)
    {
        // Create a range of notes to trigger this sample
        juce::BigInteger allNotes;
        allNotes.setRange(0, 128, true);

        // Create a new sample info and add to our list
        auto sampleIndex = sampleList.size();

        auto newSample = std::make_unique<SampleInfo>(
            file.getFileNameWithoutExtension(), file, sampleIndex);

        // Create the sampler sound
        auto samplerSound =
            new SamplerSound(file.getFileNameWithoutExtension(), *reader, allNotes);
        
        // Set the index on the sampler sound so it can be identified later
        samplerSound->setIndex(sampleIndex);
        
        newSample->sound.reset(samplerSound);

        // Add the sound to the sampler
        sampler.addSound(samplerSound);

        // Register this sound with our SamplerVoice to be accessible by index
        SamplerVoice::registerSoundWithIndex(samplerSound, sampleIndex);

        // Add to our sample list
        sampleList.push_back(std::move(newSample));

        // If it's the first sample, select it
        if (sampleList.size() == 1)
            currentSelectedSample = 0;
    }
}

void SampleManager::removeSamples(int startIdx, int endIdx)
{
    if (startIdx < 0 || endIdx < 0 || startIdx >= static_cast<int>(sampleList.size())
        || endIdx >= static_cast<int>(sampleList.size()) || startIdx > endIdx)
    {
        return;
    }

    SamplerVoice::clearSoundRegistrations();

    // Remove samples from back to front to avoid index shifting problems
    for (int i = endIdx; i >= startIdx; --i)
    {
        if (i < sampleList.size())  // Extra safety check
        {
            sampleList[i]->sound.release();
            sampleList.erase(sampleList.begin() + i);
        }
    }

    rebuildSounds();
}

void SampleManager::rebuildSounds()
{
    // Clear the sampler and all sound registrations
    sampler.clearSounds();

    // Rebuild with properly indexed sounds
    for (size_t i = 0; i < sampleList.size(); ++i)
    {
        sampleList[i]->index = i;
        SamplerSound* sound = sampleList[i]->sound.get();
        
        // Update the index on the SamplerSound as well
        if (sound != nullptr) {
            sound->setIndex(i);
            sampler.addSound(sound);
            SamplerVoice::registerSoundWithIndex(sound, i);
        }
    }

    // Update selection
    if (sampleList.empty())
    {
        currentSelectedSample = -1;
    }
    else if (currentSelectedSample >= static_cast<int>(sampleList.size()))
    {
        currentSelectedSample = sampleList.size() - 1;
    }
}

void SampleManager::clearAllSamples()
{
    sampler.clearSounds();
    sampleList.clear();
    currentSelectedSample = -1;

    // Clear our sound registration map as well
    SamplerVoice::clearSoundRegistrations();
}

int SampleManager::getNextSampleIndex(Params::DirectionType direction)
{
    if (sampleList.empty())
        return -1;

    // If only one sample, always return it
    if (sampleList.size() == 1)
        return 0;

    int nextIndex = 0;

    switch (direction)
    {
        case Params::LEFT: // Sequential backward
        {
            if (currentPlayIndex < 0)
                currentPlayIndex = currentSelectedSample >= 0 ? currentSelectedSample : 0;

            // Move backward (left)
            currentPlayIndex--;
            if (currentPlayIndex < 0)
                currentPlayIndex = sampleList.size() - 1;

            nextIndex = currentPlayIndex;
            break;
        }

        case Params::BIDIRECTIONAL: // Ping-pong between samples
        {
            if (currentPlayIndex < 0)
                currentPlayIndex = currentSelectedSample >= 0 ? currentSelectedSample : 0;

            if (isAscending)
            {
                currentPlayIndex++;
                if (currentPlayIndex >= sampleList.size())
                {
                    currentPlayIndex = sampleList.size() - 2;
                    isAscending = false;

                    // Special case for just 2 samples
                    if (currentPlayIndex < 0)
                        currentPlayIndex = 0;
                }
            }
            else
            {
                currentPlayIndex--;
                if (currentPlayIndex < 0)
                {
                    currentPlayIndex = 1;
                    isAscending = true;

                    // Special case for just 2 samples
                    if (currentPlayIndex >= sampleList.size())
                        currentPlayIndex = 0;
                }
            }

            nextIndex = currentPlayIndex;
            break;
        }

        case Params::RIGHT: // Random selection
        default:
        {
            // Choose a random sample from all samples
            nextIndex = juce::Random::getSystemRandom().nextInt(sampleList.size());
            currentPlayIndex = nextIndex;
            break;
        }
    }

    // Add debugging
    juce::String directionText;
    switch (direction)
    {
        case Params::LEFT:
            directionText = "LEFT";
            break;
        case Params::RIGHT:
            directionText = "RIGHT";
            break;
        case Params::BIDIRECTIONAL:
            directionText = "BIDIRECTIONAL";
            break;
        default:
            directionText = "UNKNOWN";
    }

    return nextIndex;
}

juce::String SampleManager::getSampleName(int index) const
{
    if (index >= 0 && index < sampleList.size())
        return sampleList[index]->name;
    return "";
}
