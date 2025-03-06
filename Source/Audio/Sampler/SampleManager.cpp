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
        int sampleIndex = sampleList.size();

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
            
            // Make sure the group index on the SamplerSound matches the SampleInfo
            int groupIndex = sampleList[i]->groupIndex;
            sound->setGroupIndex(groupIndex);
            
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

        case Params::RIGHT: // Sequential forward
        {
            if (currentPlayIndex < 0)
                currentPlayIndex = currentSelectedSample >= 0 ? currentSelectedSample : 0;

            // Move forward (right)
            currentPlayIndex++;
            if (currentPlayIndex >= sampleList.size())
                currentPlayIndex = 0;

            nextIndex = currentPlayIndex;
            break;
        }

        case Params::RANDOM: // Random selection
        default:
        {
            // Collect all ungrouped samples
            std::vector<int> ungroupedIndices;
            for (size_t i = 0; i < sampleList.size(); ++i)
            {
                if (sampleList[i]->groupIndex == -1)
                {
                    ungroupedIndices.push_back(i);
                }
            }
            
            // If groups exist, consider both groups and ungrouped samples
            if (!groups.empty())
            {
                // Calculate total probability across all groups and the "ungrouped" group
                float totalGroupProbability = 0.0f;
                
                // Add probability for each real group
                for (const auto& group : groups)
                {
                    if (!group->sampleIndices.empty())
                        totalGroupProbability += group->probability;
                }
                
                // Add a default probability for ungrouped samples
                // Use 1.0 as the default probability for the "ungrouped group"
                float ungroupedProbability = 1.0f;
                
                // Only include ungrouped probability if we have ungrouped samples
                if (!ungroupedIndices.empty())
                    totalGroupProbability += ungroupedProbability;
                
                // If we have no total probability, just pick a random sample
                if (totalGroupProbability <= 0.0f)
                {
                    nextIndex = juce::Random::getSystemRandom().nextInt(sampleList.size());
                    currentPlayIndex = nextIndex;
                    break;
                }
                
                // Randomly select a group or ungrouped samples
                float randomValue = juce::Random::getSystemRandom().nextFloat() * totalGroupProbability;
                
                float runningTotal = 0.0f;
                
                // Check if we should select from real groups
                for (const auto& group : groups)
                {
                    if (group->sampleIndices.empty())
                        continue;
                        
                    runningTotal += group->probability;
                    
                    if (randomValue <= runningTotal)
                    {
                        // We've selected this group, now pick a sample from within it
                        if (group->sampleIndices.size() == 1)
                            return group->sampleIndices[0];
                            
                        // Calculate total probability for samples in this group
                        float totalSampleProbability = 0.0f;
                        for (int idx : group->sampleIndices)
                        {
                            if (idx >= 0 && idx < sampleList.size())
                                totalSampleProbability += sampleList[idx]->probability;
                        }
                        
                        // Select a sample based on probability
                        float sampleRandomValue = juce::Random::getSystemRandom().nextFloat() * totalSampleProbability;
                        float sampleRunningTotal = 0.0f;
                        
                        for (int idx : group->sampleIndices)
                        {
                            if (idx >= 0 && idx < sampleList.size())
                            {
                                sampleRunningTotal += sampleList[idx]->probability;
                                if (sampleRandomValue <= sampleRunningTotal)
                                    return idx;
                            }
                        }
                        
                        // Fallback - shouldn't reach here
                        return group->sampleIndices[0];
                    }
                }
                
                // If we got here, we should select from ungrouped samples
                // This happens when randomValue > sum of all real group probabilities
                if (!ungroupedIndices.empty())
                {
                    if (ungroupedIndices.size() == 1)
                        return ungroupedIndices[0];
                        
                    // Calculate total probability for ungrouped samples
                    float totalUngroupedProbability = 0.0f;
                    for (int idx : ungroupedIndices)
                    {
                        totalUngroupedProbability += sampleList[idx]->probability;
                    }
                    
                    // Select a sample based on probability
                    float sampleRandomValue = juce::Random::getSystemRandom().nextFloat() * totalUngroupedProbability;
                    float sampleRunningTotal = 0.0f;
                    
                    for (int idx : ungroupedIndices)
                    {
                        sampleRunningTotal += sampleList[idx]->probability;
                        if (sampleRandomValue <= sampleRunningTotal)
                        {
                            nextIndex = idx;
                            break;
                        }
                    }
                    
                    // Fallback
                    if (nextIndex == 0 && !ungroupedIndices.empty())
                        nextIndex = ungroupedIndices[0];
                }
                else
                {
                    // No ungrouped samples, so pick from a random group
                    if (!groups.empty() && !groups[0]->sampleIndices.empty())
                        nextIndex = groups[0]->sampleIndices[0];
                    else
                        nextIndex = 0;
                }
            }
            else
            {
                // No groups, just do normal random selection across all samples
                if (sampleList.size() > 1)
                {
                    // Calculate total probability
                    float totalProbability = 0.0f;
                    for (size_t i = 0; i < sampleList.size(); ++i)
                    {
                        totalProbability += sampleList[i]->probability;
                    }
                    
                    // Select based on probability
                    float randomValue = juce::Random::getSystemRandom().nextFloat() * totalProbability;
                    float runningTotal = 0.0f;
                    
                    for (size_t i = 0; i < sampleList.size(); ++i)
                    {
                        runningTotal += sampleList[i]->probability;
                        if (randomValue <= runningTotal)
                        {
                            nextIndex = i;
                            break;
                        }
                    }
                }
                else
                {
                    nextIndex = 0;
                }
            }
            
            currentPlayIndex = nextIndex;
            break;
        }
    }

    return nextIndex;
}

juce::String SampleManager::getSampleName(int index) const
{
    if (index >= 0 && index < sampleList.size())
        return sampleList[index]->name;
    return "";
}


SamplerSound* SampleManager::getSampleSound(int index) const
{
    if (index >= 0 && index < sampleList.size())
        return sampleList[index]->sound.get();
    return nullptr;
}

juce::File SampleManager::getSampleFilePath(int index) const
{
    if (index >= 0 && index < sampleList.size())
        return sampleList[index]->file;
    return juce::File();
}

void SampleManager::createGroup(const juce::Array<int>& sampleIndices)
{
    if (sampleIndices.isEmpty() || groups.size() >= 4)
        return;
        
    // Create a new group with auto-generated name
    juce::String groupName = "Group " + juce::String(groups.size() + 1);
    int groupIndex = groups.size();
    
    auto newGroup = std::make_unique<Group>(groupName, groupIndex);
    
    // Add sample indices to the group
    for (int i = 0; i < sampleIndices.size(); ++i)
    {
        int idx = sampleIndices[i];
        if (idx >= 0 && idx < sampleList.size())
        {
            newGroup->sampleIndices.push_back(idx);
            
            // Set the group index on both the SampleInfo and SamplerSound
            sampleList[idx]->groupIndex = groupIndex;
            if (sampleList[idx]->sound != nullptr)
                sampleList[idx]->sound->setGroupIndex(groupIndex);
        }
    }
    
    // Add the group
    groups.push_back(std::move(newGroup));
}

void SampleManager::removeGroup(int groupIndex)
{
    if (groupIndex < 0 || groupIndex >= groups.size())
        return;
        
    // Remove all samples from this group
    for (size_t i = 0; i < sampleList.size(); ++i)
    {
        if (sampleList[i]->groupIndex == groupIndex)
        {
            sampleList[i]->groupIndex = -1;
            // Also update the SamplerSound
            if (sampleList[i]->sound != nullptr)
                sampleList[i]->sound->setGroupIndex(-1);
        }
    }
    
    // Remove the group
    groups.erase(groups.begin() + groupIndex);
    
    // Update remaining group indices
    for (size_t i = groupIndex; i < groups.size(); ++i)
    {
        groups[i]->index = i;
        
        // Update sample group indices
        for (size_t j = 0; j < sampleList.size(); ++j)
        {
            if (sampleList[j]->groupIndex == i + 1)
            {
                sampleList[j]->groupIndex = i;
                // Also update the SamplerSound
                if (sampleList[j]->sound != nullptr)
                    sampleList[j]->sound->setGroupIndex(i);
            }
        }
    }
}

const SampleManager::Group* SampleManager::getGroup(int index) const
{
    if (index >= 0 && index < groups.size())
        return groups[index].get();
    return nullptr;
}

void SampleManager::setSampleProbability(int sampleIndex, float probability)
{
    if (sampleIndex >= 0 && sampleIndex < sampleList.size())
    {
        // Clamp probability between 0.0 and 1.0
        sampleList[sampleIndex]->probability = juce::jlimit(0.0f, 1.0f, probability);
    }
}

float SampleManager::getSampleProbability(int sampleIndex) const
{
    if (sampleIndex >= 0 && sampleIndex < sampleList.size())
        return sampleList[sampleIndex]->probability;
    return 0.0f;
}

void SampleManager::setGroupProbability(int groupIndex, float probability)
{
    if (groupIndex >= 0 && groupIndex < groups.size())
    {
        // Clamp probability between 0.0 and 1.0
        groups[groupIndex]->probability = juce::jlimit(0.0f, 1.0f, probability);
    }
}

float SampleManager::getGroupProbability(int groupIndex) const
{
    if (groupIndex >= 0 && groupIndex < groups.size())
        return groups[groupIndex]->probability;
    return 0.0f;
}

void SampleManager::removeSampleFromGroup(int sampleIndex)
{
    if (sampleIndex < 0 || sampleIndex >= sampleList.size())
        return;
        
    // Get the current group index
    int groupIndex = -1;
    if (sampleList[sampleIndex]->sound != nullptr)
        groupIndex = sampleList[sampleIndex]->sound->getGroupIndex();
        
    // If not in a group, nothing to do
    if (groupIndex < 0 || groupIndex >= groups.size())
        return;
        
    // Remove the sample index from the group's list
    auto& sampleIndices = groups[groupIndex]->sampleIndices;
    for (size_t i = 0; i < sampleIndices.size(); ++i)
    {
        if (sampleIndices[i] == sampleIndex)
        {
            sampleIndices.erase(sampleIndices.begin() + i);
            break;
        }
    }
    
    // Update the sample's group index
    sampleList[sampleIndex]->groupIndex = -1;
    if (sampleList[sampleIndex]->sound != nullptr)
        sampleList[sampleIndex]->sound->setGroupIndex(-1);
        
    // If the group is now empty, remove it
    if (sampleIndices.empty())
        removeGroup(groupIndex);
}