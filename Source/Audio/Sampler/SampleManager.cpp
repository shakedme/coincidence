#include "SampleManager.h"
#include <algorithm>
#include <random>
#include <utility>

SampleManager::SampleManager(PluginProcessor &p) : processor(p) {
    processor.getAPVTS().addParameterListener(Params::ID_SAMPLE_DIRECTION, this);
    processor.getAPVTS().addParameterListener(Params::ID_SAMPLE_PITCH_FOLLOW, this);

    formatManager.registerBasicFormats();
    sampler.addVoice(new SamplerVoice(voiceState));
    sampler.setNoteStealingEnabled(true);
}

SampleManager::~SampleManager() {
    clearAllSamples();
}

void SampleManager::parameterChanged(const juce::String &parameterID, float newValue) {
    if (parameterID == Params::ID_SAMPLE_DIRECTION) {
        sampleDirection = static_cast<Models::DirectionType>(static_cast<int>(newValue));
    } else if (parameterID == Params::ID_SAMPLE_PITCH_FOLLOW) {
        voiceState.setPitchFollowEnabled(newValue > 0.5f);
    }
}

void SampleManager::prepareToPlay(double sampleRate) {
    sampler.setCurrentPlaybackSampleRate(sampleRate);
    sampler.allNotesOff(0, true);
}

void SampleManager::processAudio(juce::AudioBuffer<float> &buffer,
                                 juce::MidiBuffer &processedMidi) {
    int currentSampleIdx = processor.getNoteGenerator().getCurrentActiveSampleIdx();
    if (currentSampleIdx < 0 || currentSampleIdx >= getNumSamples()) {
        return;
    }

    voiceState.setCurrentSampleIndex(currentSampleIdx);
    if (currentSampleIdx != voiceState.getCurrentSampleIndex()) {
        sampler.allNotesOff(0, false);
    }

    sampler.renderNextBlock(
            buffer, processedMidi, 0, buffer.getNumSamples());
}

void SampleManager::addSample(const juce::File &file) {
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (reader != nullptr) {
        juce::BigInteger allNotes;
        allNotes.setRange(0, 128, true);

        int sampleIndex = sampleList.size();

        auto newSample = std::make_unique<SampleInfo>(
                file.getFileNameWithoutExtension(), file, sampleIndex);

        auto samplerSound =
                new SamplerSound(file.getFileNameWithoutExtension(), *reader, allNotes);

        samplerSound->setIndex(sampleIndex);

        juce::AudioBuffer<float> *audioData = samplerSound->getAudioData();
        double sampleRate = samplerSound->getSourceSampleRate();

        if (audioData != nullptr && audioData->getNumSamples() > 0) {
            std::vector<float> onsetPositions = onsetDetector.detectOnsets(*audioData, sampleRate);
            samplerSound->setOnsetMarkers(onsetPositions);
        }

        newSample->sound.reset(samplerSound);

        sampler.addSound(samplerSound);
        registerSoundWithIndex(samplerSound, sampleIndex);
        sampleList.push_back(std::move(newSample));

        if (sampleList.size() == 1)
            currentSelectedSample = 0;

        for (int i = 0; i < Models::NUM_RATE_OPTIONS; ++i) {
            updateValidSamplesForRate(static_cast<Models::RateOption>(i));
        }
    }
}

void SampleManager::removeSamples(int startIdx, int endIdx) {
    if (startIdx < 0 || endIdx < 0 || startIdx >= static_cast<int>(sampleList.size())
        || endIdx >= static_cast<int>(sampleList.size()) || startIdx > endIdx) {
        return;
    }

    // Ensure no sounds are playing that might reference these samples
    sampler.allNotesOff(0, true);

    // Clear the sound registrations first to avoid dangling pointers
    clearSoundRegistrations();

    // Remove samples from back to front to avoid index shifting problems
    for (int i = endIdx; i >= startIdx; --i) {
        if (i < sampleList.size())  // Extra safety check
        {
            // Remove from any groups before deleting
            if (sampleList[i]->groupIndex >= 0) {
                removeSampleFromGroup(i);
            }

            // Clear the sound and release memory
            sampleList[i]->sound.release();
            sampleList.erase(sampleList.begin() + i);
        }
    }

    rebuildSounds();

    // Update valid samples lists for all rates since we removed samples
    for (int i = 0; i < Models::NUM_RATE_OPTIONS; ++i) {
        updateValidSamplesForRate(static_cast<Models::RateOption>(i));
    }
}

void SampleManager::rebuildSounds() {
    // Clear the sampler and all sound registrations
    sampler.clearSounds();

    // Rebuild with properly indexed sounds
    for (size_t i = 0; i < sampleList.size(); ++i) {
        sampleList[i]->index = i;
        SamplerSound *sound = sampleList[i]->sound.get();

        // Update the index on the SamplerSound as well
        if (sound != nullptr) {
            sound->setIndex(i);

            // Make sure the group index on the SamplerSound matches the SampleInfo
            int groupIndex = sampleList[i]->groupIndex;
            sound->setGroupIndex(groupIndex);

            sampler.addSound(sound);
            registerSoundWithIndex(sound, i);
        }
    }

    // Update selection
    if (sampleList.empty()) {
        currentSelectedSample = -1;
    } else if (currentSelectedSample >= static_cast<int>(sampleList.size())) {
        currentSelectedSample = sampleList.size() - 1;
    }
}

void SampleManager::clearAllSamples() {
    // Stop all playing notes first to avoid references to deleted samples
    sampler.allNotesOff(0, true);

    // Clear the sounds from the sampler - this will delete the SamplerSound objects
    sampler.clearSounds();

    // Clear our sound registration map before clearing the sample list
    // to avoid any dangling pointers
    clearSoundRegistrations();

    // Clear all groups
    groups.clear();

    // Finally clear the sample list
    sampleList.clear();
    currentSelectedSample = -1;
    currentPlayIndex = -1;
    isAscending = true;

    // Clear all valid samples
    validSamplesForRate.clear();
}

int SampleManager::getNextSampleIndex(Models::RateOption currentRate) {
    const auto &validSamples = getValidSamplesForRate(currentRate);

    if (validSamples.empty())
        return -1;

    // If only one valid sample, always return it
    if (validSamples.size() == 1 && getSampleProbability(validSamples[0]) > 0.0f)
        return validSamples[0];

    // Find the current play index in the valid samples list
    int currentValidIndex = -1;
    if (currentPlayIndex >= 0) {
        for (size_t i = 0; i < validSamples.size(); ++i) {
            if (validSamples[i] == currentPlayIndex) {
                currentValidIndex = i;
                break;
            }
        }
    }

    // If current index not found, start from beginning
    if (currentValidIndex < 0) {
        currentValidIndex = 0;
    }

    int nextValidIndex = currentValidIndex;
    switch (sampleDirection) {
        case Models::LEFT: // Sequential backward
        {
            nextValidIndex = (currentValidIndex - 1 + validSamples.size()) % validSamples.size();
            break;
        }

        case Models::BIDIRECTIONAL: // Ping-pong between samples
        {
            if (isAscending) {
                nextValidIndex = currentValidIndex + 1;
                if (nextValidIndex >= validSamples.size()) {
                    nextValidIndex = validSamples.size() - 2;
                    isAscending = false;
                    if (nextValidIndex < 0)
                        nextValidIndex = 0;
                }
            } else {
                nextValidIndex = currentValidIndex - 1;
                if (nextValidIndex < 0) {
                    nextValidIndex = 1;
                    isAscending = true;
                    if (nextValidIndex >= validSamples.size())
                        nextValidIndex = 0;
                }
            }
            break;
        }

        case Models::RIGHT: // Sequential forward
        {
            nextValidIndex = (currentValidIndex + 1) % validSamples.size();
            break;
        }

        case Models::RANDOM: // Random selection with probability
        {
            // Use more reliable random selection logic
            std::random_device rd;
            std::mt19937 gen(rd() + juce::Random::getSystemRandom().nextInt64());
            
            // First check if there are any samples with non-zero probability
            bool hasNonZeroProbability = false;
            for (int idx : validSamples) {
                if (getSampleProbability(idx) > 0.0f) {
                    hasNonZeroProbability = true;
                    break;
                }
            }
            
            if (!hasNonZeroProbability) {
                return -1; // No samples with non-zero probability
            }
            
            // Try to avoid consecutive repeats in random mode
            int previousIndex = currentPlayIndex;
            int result;
            
            // If we have more than one valid sample, try to avoid playing the same sample twice
            if (validSamples.size() > 1) {
                int attempts = 0;
                do {
                    result = selectRandomSampleWithProbability(validSamples);
                    attempts++;
                    // Only try a limited number of times to avoid infinite loops
                    // if there's only one sample with non-zero probability
                } while (result == previousIndex && attempts < 3 && result >= 0);
            } else {
                result = selectRandomSampleWithProbability(validSamples);
            }
            
            // If no valid samples with non-zero probability, return -1 to indicate don't play anything
            if (result == -1) {
                return -1; // Signal not to play anything
            }
            
            // Update index and return
            return result;
        }
    }

    // Update current play index and return the actual sample index
    currentPlayIndex = validSamples[nextValidIndex];
    return currentPlayIndex;
}

int SampleManager::selectRandomSampleWithProbability(const std::vector<int> &validSamples) {
    if (validSamples.empty()) {
        return 0;
    }

    // Filter out samples with zero probability first
    std::vector<int> samplesWithProbability;
    for (int idx: validSamples) {
        if (getSampleProbability(idx) > 0.0f) {
            samplesWithProbability.push_back(idx);
        }
    }

    // If no samples have probability > 0, return -1 to indicate no valid sample
    if (samplesWithProbability.empty()) {
        return -1; // No valid samples with non-zero probability
    }

    // Group valid samples by their group
    std::map<int, std::vector<int>> groupedValidSamples;
    float totalGroupProbability = 0.0f;

    // Organize valid samples by group and calculate total group probability
    organizeValidSamplesByGroup(samplesWithProbability, groupedValidSamples, totalGroupProbability);

    if (totalGroupProbability > 0.0f) {
        return selectFromGroupedSamples(groupedValidSamples, totalGroupProbability);
    }

    return -1;
}

void SampleManager::organizeValidSamplesByGroup(const std::vector<int> &validSamples,
                                                std::map<int, std::vector<int>> &groupedValidSamples,
                                                float &totalGroupProbability) {
    // First pass: organize valid samples by group and calculate total group probability
    for (int idx: validSamples) {
        int groupIdx = sampleList[idx]->groupIndex;
        // Add to appropriate group (including ungrouped samples which have groupIndex = -1)
        groupedValidSamples[groupIdx].push_back(idx);

        // Add the group probability once per group
        if (groupedValidSamples[groupIdx].size() == 1) {
            totalGroupProbability += getGroupProbability(groupIdx);
        }
    }
}

int SampleManager::selectFromGroupedSamples(const std::map<int, std::vector<int>> &groupedValidSamples,
                                            float totalGroupProbability) {
    // STEP 1: Select a group based on group probability
    int selectedGroupIdx = selectGroup(groupedValidSamples, totalGroupProbability);

    // STEP 2: Select a sample from the chosen group
    if (!groupedValidSamples.at(selectedGroupIdx).empty()) {
        return selectSampleFromGroup(groupedValidSamples.at(selectedGroupIdx));
    }

    // If all attempts failed, return -1
    return -1;
}

int SampleManager::selectGroup(const std::map<int, std::vector<int>> &groupedValidSamples,
                               float totalGroupProbability) {

    if (groupedValidSamples.size() == 1) {
        // Only one group, return it
        return groupedValidSamples.begin()->first;
    }

    // Create a vector of pairs (groupIdx, probability) for unbiased selection
    std::vector<std::pair<int, float>> groupProbabilities;
    for (const auto &[groupIdx, samples]: groupedValidSamples) {
        float probability = getGroupProbability(groupIdx);
        if (probability > 0.0f) {
            groupProbabilities.emplace_back(groupIdx, probability);
        }
    }

    // If no groups with non-zero probability, return -1
    if (groupProbabilities.empty()) {
        return -1;
    }

    // Use a proper generator with proper distribution instead of biased approaches
    std::random_device rd;
    std::mt19937 gen(rd() + juce::Random::getSystemRandom().nextInt64());

    // Check for equal probabilities case - if all groups have the same probability
    bool allEqual = true;
    float firstProb = groupProbabilities[0].second;
    for (size_t i = 1; i < groupProbabilities.size(); ++i) {
        if (std::abs(groupProbabilities[i].second - firstProb) > 0.0001f) {
            allEqual = false;
            break;
        }
    }

    // If all probabilities are equal, just use uniform distribution
    if (allEqual) {
        std::uniform_int_distribution<> distrib(0, groupProbabilities.size() - 1);
        int randomIndex = distrib(gen);
        return groupProbabilities[randomIndex].first;
    }

    std::vector<float> weights;
    weights.reserve(groupProbabilities.size());
    for (const auto& pair : groupProbabilities) {
        weights.push_back(pair.second);
    }
    
    std::discrete_distribution<> distribution(weights.begin(), weights.end());
    int selectedIndex = distribution(gen);
    return groupProbabilities[selectedIndex].first;
}

int SampleManager::selectSampleFromGroup(const std::vector<int> &samplesInGroup) {
    // Calculate total probability for samples in this group
    std::vector<std::pair<int, float>> sampleProbabilities;

    for (int idx: samplesInGroup) {
        float prob = getSampleProbability(idx);
        sampleProbabilities.emplace_back(idx, prob);
    }

    // If there are no samples with probability, return -1
    if (sampleProbabilities.empty()) {
        return -1;
    }

    // Use a proper generator with proper distribution
    std::random_device rd;
    std::mt19937 gen(rd() + juce::Random::getSystemRandom().nextInt64());

    // Check for equal probabilities case
    bool allEqual = true;
    float firstProb = sampleProbabilities[0].second;
    for (size_t i = 1; i < sampleProbabilities.size(); ++i) {
        if (std::abs(sampleProbabilities[i].second - firstProb) > 0.0001f) {
            allEqual = false;
            break;
        }
    }

    // If all probabilities are equal, just use uniform distribution
    if (allEqual) {
        std::uniform_int_distribution<> distrib(0, sampleProbabilities.size() - 1);
        int randomIndex = distrib(gen);
        return sampleProbabilities[randomIndex].first;
    }

    // Use discrete distribution for weighted random selection
    // Extract just the probability values for the distribution
    std::vector<float> weights;
    weights.reserve(sampleProbabilities.size());
    for (const auto& pair : sampleProbabilities) {
        weights.push_back(pair.second);
    }
    
    std::discrete_distribution<> distribution(weights.begin(), weights.end());
    int selectedIndex = distribution(gen);
    return sampleProbabilities[selectedIndex].first;
}

juce::String SampleManager::getSampleName(int index) const {
    if (index >= 0 && index < sampleList.size())
        return sampleList[index]->name;
    return "";
}


SamplerSound *SampleManager::getSampleSound(int index) const {
    if (index >= 0 && index < sampleList.size())
        return sampleList[index]->sound.get();
    return nullptr;
}

juce::File SampleManager::getSampleFilePath(int index) const {
    if (index >= 0 && index < sampleList.size())
        return sampleList[index]->file;
    return {};
}

void SampleManager::createGroup(const juce::Array<int> &sampleIndices) {
    if (sampleIndices.isEmpty() || groups.size() >= 4)
        return;

    // Create a new group with auto-generated name
    juce::String groupName = "Group " + juce::String(groups.size() + 1);
    int groupIndex = groups.size();

    auto newGroup = std::make_unique<Group>(groupName, groupIndex);

    // Add sample indices to the group
    for (int idx: sampleIndices) {
        if (idx >= 0 && idx < sampleList.size()) {
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

void SampleManager::removeGroup(int groupIndex) {
    if (groupIndex < 0 || groupIndex >= groups.size())
        return;

    // Remove all samples from this group
    for (const auto &i: sampleList) {
        if (i->groupIndex == groupIndex) {
            i->groupIndex = -1;
            // Also update the SamplerSound
            if (i->sound != nullptr)
                i->sound->setGroupIndex(-1);
        }
    }

    // Remove the group
    groups.erase(groups.begin() + groupIndex);

    // Update remaining group indices
    for (size_t i = groupIndex; i < groups.size(); ++i) {
        groups[i]->index = i;

        // Update sample group indices
        for (const auto &j: sampleList) {
            if (j->groupIndex == i + 1) {
                j->groupIndex = i;
                // Also update the SamplerSound
                if (j->sound != nullptr)
                    j->sound->setGroupIndex(i);
            }
        }
    }
}

const SampleManager::Group *SampleManager::getGroup(int index) const {
    if (index >= 0 && index < groups.size())
        return groups[index].get();
    return nullptr;
}

void SampleManager::setSampleProbability(int sampleIndex, float probability) {
    if (sampleIndex >= 0 && sampleIndex < sampleList.size()) {
        // Clamp probability between 0.0 and 1.0
        sampleList[sampleIndex]->probability = juce::jlimit(0.0f, 1.0f, probability);
    }
}

float SampleManager::getSampleProbability(int sampleIndex) const {
    if (sampleIndex >= 0 && sampleIndex < sampleList.size())
        return sampleList[sampleIndex]->probability;
    return 0.0f;
}

void SampleManager::setGroupProbability(int groupIndex, float probability) {
    if (groupIndex >= 0 && groupIndex < groups.size()) {
        // Clamp probability between 0.0 and 1.0
        groups[groupIndex]->probability = juce::jlimit(0.0f, 1.0f, probability);
    }
}

float SampleManager::getGroupProbability(int groupIndex) const {
    if (groupIndex >= 0 && groupIndex < groups.size())
        return groups[groupIndex]->probability;
    return 1.0f;
}

void SampleManager::removeSampleFromGroup(int sampleIndex) {
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
    auto &sampleIndices = groups[groupIndex]->sampleIndices;
    for (size_t i = 0; i < sampleIndices.size(); ++i) {
        if (sampleIndices[i] == sampleIndex) {
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

void SampleManager::setSampleRateEnabled(int sampleIndex, Models::RateOption rate, bool enabled) {
    if (sampleIndex >= 0 && sampleIndex < sampleList.size()) {
        auto &sample = sampleList[sampleIndex];
        sample->rateEnabled[rate] = enabled;

        // Update the valid samples list for this rate
        updateValidSamplesForRate(rate);
    }
}

bool SampleManager::isSampleRateEnabled(int sampleIndex, Models::RateOption rate) const {
    if (sampleIndex >= 0 && sampleIndex < sampleList.size()) {
        const auto &sample = sampleList[sampleIndex];

        // group state is more important than individual sample state
        if (sample->groupIndex >= 0 && sample->groupIndex < groups.size()) {
            return isGroupRateEnabled(sample->groupIndex, rate);
        }

        auto it = sample->rateEnabled.find(rate);
        if (it == sample->rateEnabled.end() || !it->second) {
            return false;
        }

        return true;
    }
    return false;
}

void SampleManager::updateValidSamplesForRate(Models::RateOption rate) {
    // Clear the vector for this rate (creates it if it doesn't exist)
    validSamplesForRate[rate].clear();

    // Add all valid samples for this rate
    for (size_t i = 0; i < sampleList.size(); ++i) {
        if (isSampleRateEnabled(i, rate)) {
            validSamplesForRate[rate].push_back(i);
        }
    }
}

const std::vector<int> &SampleManager::getValidSamplesForRate(Models::RateOption rate) const {
    // If the rate exists in the map, return its vector
    auto it = validSamplesForRate.find(rate);
    if (it != validSamplesForRate.end()) {
        return it->second;
    }

    // Fallback to an empty vector if not found
    static const std::vector<int> emptyVector;
    return emptyVector;
}

// Group rate methods implementation
void SampleManager::setGroupRateEnabled(int groupIndex, Models::RateOption rate, bool enabled) {
    if (groupIndex >= 0 && groupIndex < groups.size()) {
        auto &group = groups[groupIndex];
        group->rateEnabled[rate] = enabled;
        updateValidSamplesForRate(rate);
    }
}

bool SampleManager::isGroupRateEnabled(int groupIndex, Models::RateOption rate) const {
    if (groupIndex >= 0 && groupIndex < groups.size()) {
        const auto &group = groups[groupIndex];
        auto it = group->rateEnabled.find(rate);
        return it != group->rateEnabled.end() && it->second;
    }
    return false;
}

void SampleManager::normalizeSamples() {
    if (sampleList.empty()) return;

    // Find global peak and store individual sample peaks
    float globalPeak = 0.0f;
    std::vector<float> samplePeaks(sampleList.size(), 0.0f);

    // First pass - find peaks
    for (size_t i = 0; i < sampleList.size(); ++i) {
        auto &sample = sampleList[i];
        if (!sample->sound) continue;

        juce::AudioBuffer<float> *audioData = sample->sound->getAudioData();
        if (!audioData) continue;

        // Find peak across all channels
        for (int channel = 0; channel < audioData->getNumChannels(); ++channel) {
            float channelPeak = audioData->getMagnitude(channel, 0, audioData->getNumSamples());
            samplePeaks[i] = std::max(samplePeaks[i], channelPeak);
        }

        globalPeak = std::max(globalPeak, samplePeaks[i]);
    }

    if (globalPeak <= 0.0001f) return; // Avoid division by very small numbers

    // Target level (using -0.5dB as suggested)
    const float targetLevel = 0.95f;
    const float globalGain = targetLevel / globalPeak;

    // Second pass - apply normalization
    for (auto &sample: sampleList) {
        if (!sample->sound) continue;

        juce::AudioBuffer<float> *audioData = sample->sound->getAudioData();
        if (!audioData) continue;

        // Apply same gain to all samples to maintain relative levels
        audioData->applyGain(globalGain);
    }
}