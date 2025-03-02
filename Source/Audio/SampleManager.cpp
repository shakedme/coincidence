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
    
    // Set up synth voices
    for (int i = 0; i < 16; ++i)
        sampler.addVoice(new SamplerVoice());

    sampler.setNoteStealingEnabled(true);
}

SampleManager::~SampleManager()
{
    clearAllSamples();
}

void SampleManager::prepareToPlay(double sampleRate)
{
    sampler.setCurrentPlaybackSampleRate(sampleRate);
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
        newSample->sound.reset(samplerSound);

        // Add the sound to the sampler
        sampler.addSound(samplerSound);

        // Add to our sample list
        sampleList.push_back(std::move(newSample));

        // If it's the first sample, select it
        if (sampleList.size() == 1)
            currentSelectedSample = 0;
    }
}

void SampleManager::removeSample(int index)
{
    if (index < 0 || index >= static_cast<int>(sampleList.size()))
    {
        return;
    }

    // First, get a reference to the sound
    SamplerSound* soundToRemove = sampleList[index]->sound.get();

    // Clear the sound from the sampler first to avoid use-after-free
    sampler.clearSounds();

    // IMPORTANT: Release ownership of the pointer before erasing from vector
    // This prevents the unique_ptr from trying to delete the incomplete type
    sampleList[index]->sound.release();

    // Now it's safe to erase the element from the vector
    sampleList.erase(sampleList.begin() + index);

    // Rebuild the sampler and renumber indices
    for (size_t i = 0; i < sampleList.size(); ++i)
    {
        sampleList[i]->index = i;
        sampler.addSound(sampleList[i]->sound.get());
    }

    // Update current selection
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
}

void SampleManager::selectSample(int index)
{
    if (index >= 0 && index < sampleList.size())
    {
        currentSelectedSample = index;
    }
}

int SampleManager::getNextSampleIndex(bool useRandomSample, float randomizeProbability)
{
    if (sampleList.empty())
        return -1;

    if (!useRandomSample || sampleList.size() == 1)
        return currentSelectedSample;

    // Check if we should randomize based on probability
    if (juce::Random::getSystemRandom().nextFloat() * 100.0f < randomizeProbability)
    {
        // Choose a random sample from all samples
        return juce::Random::getSystemRandom().nextInt(sampleList.size());
    }

    // Default to current selection
    return currentSelectedSample;
}

juce::String SampleManager::getSampleName(int index) const
{
    if (index >= 0 && index < sampleList.size())
        return sampleList[index]->name;
    return "";
}
