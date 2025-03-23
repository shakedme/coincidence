#include "SampleManager.h"
#include <algorithm>
#include <random>
#include <utility>

SampleManager::SampleInfo::SampleInfo(juce::String n, juce::File f, int idx)
        : name(std::move(n)), file(std::move(f)), index(idx) {
    // Initialize all rates to enabled by default
    for (int i = 0; i < Models::NUM_RATE_OPTIONS; ++i) {
        rateEnabled[static_cast<Models::RateOption>(i)] = true;
    }
}

SampleManager::SampleManager(PluginProcessor &p) : processor(p) {
    processor.getAPVTS().addParameterListener(AppState::ID_SAMPLE_DIRECTION, this);
    processor.getAPVTS().addParameterListener(AppState::ID_SAMPLE_PITCH_FOLLOW, this);
    processor.getAPVTS().addParameterListener(AppState::ID_ADSR_ATTACK, this);
    processor.getAPVTS().addParameterListener(AppState::ID_ADSR_DECAY, this);
    processor.getAPVTS().addParameterListener(AppState::ID_ADSR_SUSTAIN, this);
    processor.getAPVTS().addParameterListener(AppState::ID_ADSR_RELEASE, this);

    formatManager.registerBasicFormats();

    // Set up synth voices - increase from 32 to 64 voices for more polyphony
    for (int i = 0; i < 64; ++i) {
        auto *voice = new SamplerVoice();
        voice->setVoiceState(&voiceState); // Set the voice state
        sampler.addVoice(voice);
    }

    // Enable voice stealing to handle when all voices are in use
    sampler.setNoteStealingEnabled(true);
}

SampleManager::~SampleManager() {
    clearAllSamples();
}

void SampleManager::parameterChanged(const juce::String &parameterID, float newValue) {
    if (parameterID == AppState::ID_SAMPLE_DIRECTION) {
        sampleDirection = static_cast<Models::DirectionType>(static_cast<int>(newValue));
    } else if (parameterID == AppState::ID_SAMPLE_PITCH_FOLLOW) {
        voiceState.setPitchFollowEnabled(newValue > 0.5f);
    } else if (parameterID == AppState::ID_ADSR_ATTACK) {
        float attackMs = newValue * 5000.0f;
        auto params = voiceState.getADSRParameters();
        voiceState.setADSRParameters(attackMs, params.decay * 1000.0f, params.sustain, params.release * 1000.0f);
    } else if (parameterID == AppState::ID_ADSR_DECAY) {
        float decayMs = newValue * 5000.0f;
        auto params = voiceState.getADSRParameters();
        voiceState.setADSRParameters(params.attack * 1000.0f, decayMs, params.sustain, params.release * 1000.0f);
    } else if (parameterID == AppState::ID_ADSR_SUSTAIN) {
        float sustain = newValue;
        auto params = voiceState.getADSRParameters();
        voiceState.setADSRParameters(params.attack * 1000.0f, params.decay * 1000.0f, sustain,
                                     params.release * 1000.0f);
    } else if (parameterID == AppState::ID_ADSR_RELEASE) {
        float releaseMs = newValue * 5000.0f;
        auto params = voiceState.getADSRParameters();
        voiceState.setADSRParameters(params.attack * 1000.0f, params.decay * 1000.0f, params.sustain, releaseMs);
    }
}

void SampleManager::prepareToPlay(double sampleRate) {
    sampler.setCurrentPlaybackSampleRate(sampleRate);

    // Reset all voices to ensure they're in a clean state
    sampler.allNotesOff(0, true);
}

void SampleManager::processAudio(juce::AudioBuffer<float> &buffer,
                                 juce::MidiBuffer &processedMidi) {
    // Get the current active sample index from the note generator
    int currentSampleIdx = processor.getNoteGenerator().getCurrentActiveSampleIdx();

    static int lastLoggedSampleIdx = -1;
    if (currentSampleIdx != lastLoggedSampleIdx) {
        lastLoggedSampleIdx = currentSampleIdx;
    }

    // Set the global sample index directly
    if (currentSampleIdx >= 0 && currentSampleIdx < getNumSamples()) {
        // Update the sample index in the voice state
        setCurrentSampleIndex(currentSampleIdx);

        // If the sample index has changed, we need to stop any active notes
        // to make sure we use the new sample for upcoming notes
        static int lastPlayedSampleIdx = -1;
        if (currentSampleIdx != lastPlayedSampleIdx) {
            // Stop all notes but don't reset voices completely
            // This allows for quicker sample switching without audio dropouts
            sampler.allNotesOff(0, false);

            lastPlayedSampleIdx = currentSampleIdx;
        }
    }

    // Create a new MIDI buffer with modified messages that include the sample index
    juce::MidiBuffer modifiedMidi;

    // Only process if we have a valid sample index
    if (currentSampleIdx >= 0 && currentSampleIdx < getNumSamples()) {
        // Process each MIDI message in the buffer
        for (const auto metadata: processedMidi) {
            auto msg = metadata.getMessage();
            const int samplePosition = metadata.samplePosition;

            // Only modify note-on messages - this will trigger sample playback
            if (msg.isNoteOn()) {
                // Create a new note-on message with the note number and the sample index in the MIDI channel
                // This will make the sample at currentSampleIdx play for this note
                int channel = 1; // Default MIDI channel 1
                int noteNumber = msg.getNoteNumber();
                int velocity = msg.getVelocity();

                // Create a MIDI message that our sampler will use to play the right sample
                juce::MidiMessage noteOnMsg = juce::MidiMessage::noteOn(channel, noteNumber, (juce::uint8) velocity);

                // Create a controller change message for controller 32 with the sample index value
                juce::MidiMessage controllerMsg = juce::MidiMessage::controllerEvent(channel, 32, currentSampleIdx);

                // Log whenever we send a controller message for sample selection
                static int lastLoggedControllerValue = -1;
                if (currentSampleIdx != lastLoggedControllerValue) {
                    lastLoggedControllerValue = currentSampleIdx;
                }

                // Add both messages to our new buffer - controller first, then note
                modifiedMidi.addEvent(controllerMsg, samplePosition);
                modifiedMidi.addEvent(noteOnMsg, samplePosition);
            } else if (msg.isNoteOff()) {
                // Pass through note-off messages to properly end notes
                modifiedMidi.addEvent(msg, samplePosition);
            } else {
                // Pass through all other messages
                modifiedMidi.addEvent(msg, samplePosition);
            }
        }
    } else {
        // If we don't have a valid index, still process note-offs to avoid stuck notes
        for (const auto metadata: processedMidi) {
            auto msg = metadata.getMessage();
            if (msg.isNoteOff()) {
                modifiedMidi.addEvent(msg, metadata.samplePosition);
            }
        }
    }

    // Use JUCE's synthesizer to render the audio with our modified MIDI
    sampler.renderNextBlock(
            buffer, modifiedMidi, 0, buffer.getNumSamples());
}

void SampleManager::addSample(const juce::File &file) {
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (reader != nullptr) {
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

        juce::AudioBuffer<float> *audioData = samplerSound->getAudioData();
        double sampleRate = samplerSound->getSourceSampleRate();

        if (audioData != nullptr && audioData->getNumSamples() > 0) {
            std::vector<float> onsetPositions = onsetDetector.detectOnsets(*audioData, sampleRate);
            samplerSound->setOnsetMarkers(onsetPositions);
        }

        newSample->sound.reset(samplerSound);

        // Add the sound to the sampler
        sampler.addSound(samplerSound);

        // Register this sound with our voice state
        registerSoundWithIndex(samplerSound, sampleIndex);

        // Add to our sample list
        sampleList.push_back(std::move(newSample));

        // If it's the first sample, select it
        if (sampleList.size() == 1)
            currentSelectedSample = 0;

        // Update valid samples lists for all rates since we added a new sample
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
            int result = selectRandomSampleWithProbability(validSamples);
            // If no valid samples with non-zero probability, return -1 to indicate don't play anything
            if (result == -1) {
                return -1; // Signal not to play anything
            }
            nextValidIndex = result;
            break;
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

    // Check for equal probabilities case - if all groups have the same probability
    // we can just select one at random without roulette wheel selection
    bool allEqual = true;
    float firstProb = groupProbabilities[0].second;
    for (size_t i = 1; i < groupProbabilities.size(); ++i) {
        if (std::abs(groupProbabilities[i].second - firstProb) > 0.0001f) {
            allEqual = false;
            break;
        }
    }

    // If all probabilities are equal, just pick randomly
    if (allEqual) {
        // Shuffle first for true randomness
        std::shuffle(groupProbabilities.begin(), groupProbabilities.end(),
                     std::default_random_engine(juce::Random::getSystemRandom().nextInt64()));

        return groupProbabilities[0].first;
    }

    // True roulette wheel selection (doesn't depend on iteration order at all)
    float randomValue = juce::Random::getSystemRandom().nextFloat() * totalGroupProbability;
    float cumulativeProbability = 0.0f;

    // Sort the vector randomly to avoid any potential bias
    std::shuffle(groupProbabilities.begin(), groupProbabilities.end(),
                 std::default_random_engine(juce::Random::getSystemRandom().nextInt64()));

    // Implementation of roulette wheel selection
    for (const auto &[groupIdx, probability]: groupProbabilities) {
        cumulativeProbability += probability;
        if (randomValue <= cumulativeProbability) {
            return groupIdx;
        }
    }

    // Fallback if no group selected (could happen due to floating-point precision issues)
    if (!groupProbabilities.empty()) {
        return groupProbabilities.back().first;
    }

    return -1;
}

int SampleManager::selectSampleFromGroup(const std::vector<int> &samplesInGroup) {
    // Calculate total probability for samples in this group
    float totalSampleProbability = 0.0f;
    std::vector<std::pair<int, float>> sampleProbabilities;

    for (int idx: samplesInGroup) {
        float prob = getSampleProbability(idx);
        totalSampleProbability += prob;
        sampleProbabilities.emplace_back(idx, prob);
    }

    // Check for equal probabilities case
    bool allEqual = true;
    if (!sampleProbabilities.empty()) {
        float firstProb = sampleProbabilities[0].second;
        for (size_t i = 1; i < sampleProbabilities.size(); ++i) {
            if (std::abs(sampleProbabilities[i].second - firstProb) > 0.0001f) {
                allEqual = false;
                break;
            }
        }
    }

    // If all probabilities are equal, just pick randomly
    if (allEqual && !sampleProbabilities.empty()) {
        // Shuffle first for true randomness
        std::shuffle(sampleProbabilities.begin(), sampleProbabilities.end(),
                     std::default_random_engine(juce::Random::getSystemRandom().nextInt64()));

        return sampleProbabilities[0].first;
    }

    // Shuffle the probabilities vector to remove bias when probabilities are equal
    std::shuffle(sampleProbabilities.begin(), sampleProbabilities.end(),
                 std::default_random_engine(juce::Random::getSystemRandom().nextInt64()));

    // Select based on sample probability
    float randomSampleValue = juce::Random::getSystemRandom().nextFloat() * totalSampleProbability;
    float runningSampleTotal = 0.0f;

    for (const auto &[idx, probability]: sampleProbabilities) {
        runningSampleTotal += probability;
        if (randomSampleValue <= runningSampleTotal) {
            return idx;
        }
    }

    // Fallback in case of rounding errors
    return samplesInGroup.back();
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
    for (size_t i = 0; i < sampleList.size(); ++i) {
        auto &sample = sampleList[i];
        if (!sample->sound) continue;

        juce::AudioBuffer<float> *audioData = sample->sound->getAudioData();
        if (!audioData) continue;

        // Apply same gain to all samples to maintain relative levels
        audioData->applyGain(globalGain);
    }
}

void SampleManager::setMaxPlayDurationForSample(juce::int64 durationInSamples) {
    voiceState.setMaxPlayDuration(durationInSamples);
}
