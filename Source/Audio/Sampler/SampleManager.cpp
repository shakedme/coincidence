#include "SampleManager.h"

SampleManager::SampleInfo::SampleInfo(const juce::String &n, const juce::File &f, int idx)
        : name(n), file(f), index(idx) {
}

SampleManager::SampleManager(PluginProcessor& p): processor(p) {
    // Initialize format manager
    formatManager.registerBasicFormats();

    // Set up synth voices - increase from 32 to 64 voices for more polyphony
    for (int i = 0; i < 64; ++i)
        sampler.addVoice(new SamplerVoice());

    // Enable voice stealing to handle when all voices are in use
    sampler.setNoteStealingEnabled(true);
}

SampleManager::~SampleManager() {
    clearAllSamples();
}

void SampleManager::prepareToPlay(double sampleRate) {
    sampler.setCurrentPlaybackSampleRate(sampleRate);

    // Reset all voices to ensure they're in a clean state
    sampler.allNotesOff(0, true);
}

void SampleManager::processAudio(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& processedMidi,
                                             juce::MidiBuffer& midiMessages)
{
    // If we have samples loaded, process the MIDI through our sampler
    if (isSampleLoaded())
    {
//        // Increment our counter and occasionally do a full voice cleanup
//        // to prevent voice leaks that could cause dropouts
//        bufferCounter++;
//        if (bufferCounter >= 1000) { // About every few seconds depending on buffer size
//            sampleManager.getSampler().allNotesOff(0, false);
//            bufferCounter = 0;
//        }

        // Get the current active sample index from the note generator
        int currentSampleIdx = processor.getNoteGenerator().getCurrentActiveSampleIdx();

        static int lastLoggedSampleIdx = -1;
        if (currentSampleIdx != lastLoggedSampleIdx) {
            lastLoggedSampleIdx = currentSampleIdx;
        }

        // Set the global sample index directly
        if (currentSampleIdx >= 0 && currentSampleIdx < getNumSamples()) {
            // Update the global index that all voices will check
            SamplerVoice::setCurrentSampleIndex(currentSampleIdx);

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
            for (const auto metadata : processedMidi) {
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
                    juce::MidiMessage noteOnMsg = juce::MidiMessage::noteOn(channel, noteNumber, (juce::uint8)velocity);

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
                }
                else if (msg.isNoteOff()) {
                    // Pass through note-off messages to properly end notes
                    modifiedMidi.addEvent(msg, samplePosition);
                }
                else {
                    // Pass through all other messages
                    modifiedMidi.addEvent(msg, samplePosition);
                }
            }
        }
        else {
            // If we don't have a valid index, still process note-offs to avoid stuck notes
            for (const auto metadata : processedMidi) {
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
    else
    {
        // If no samples are loaded, pass through our generated MIDI
        midiMessages.swapWith(processedMidi);
    }
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

        // Register this sound with our SamplerVoice to be accessible by index
        SamplerVoice::registerSoundWithIndex(samplerSound, sampleIndex);

        // Add to our sample list
        sampleList.push_back(std::move(newSample));

        // If it's the first sample, select it
        if (sampleList.size() == 1)
            currentSelectedSample = 0;

        // Update valid samples lists for all rates since we added a new sample
        for (int i = 0; i < Params::NUM_RATE_OPTIONS; ++i) {
            updateValidSamplesForRate(static_cast<Params::RateOption>(i));
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
    SamplerVoice::clearSoundRegistrations();

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
    for (int i = 0; i < Params::NUM_RATE_OPTIONS; ++i) {
        updateValidSamplesForRate(static_cast<Params::RateOption>(i));
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
            SamplerVoice::registerSoundWithIndex(sound, i);
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
    SamplerVoice::clearSoundRegistrations();

    // Clear all groups
    groups.clear();

    // Finally clear the sample list
    sampleList.clear();
    currentSelectedSample = -1;
    currentPlayIndex = -1;
    isAscending = true;

    // Clear all valid samples lists
    validSamples_1_2.clear();
    validSamples_1_4.clear();
    validSamples_1_8.clear();
    validSamples_1_16.clear();
}

int SampleManager::getNextSampleIndex(Params::DirectionType direction, Params::RateOption currentRate) {
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
    switch (direction) {
        case Params::LEFT: // Sequential backward
        {
            nextValidIndex = (currentValidIndex - 1 + validSamples.size()) % validSamples.size();
            break;
        }

        case Params::BIDIRECTIONAL: // Ping-pong between samples
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

        case Params::RIGHT: // Sequential forward
        {
            nextValidIndex = (currentValidIndex + 1) % validSamples.size();
            break;
        }

        case Params::RANDOM: // Random selection with probability
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
        return selectFromGroupedSamples(groupedValidSamples, totalGroupProbability,
                                        samplesWithProbability);
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
                                            float totalGroupProbability,
                                            const std::vector<int> &validSamples) {
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

    float randomValue = juce::Random::getSystemRandom().nextFloat() * totalGroupProbability;

    float runningGroupTotal = 0.0f;
    // Iterate through all real groups
    for (const auto &[groupIdx, samples]: groupedValidSamples) {
        runningGroupTotal += getGroupProbability(groupIdx);
        if (randomValue <= runningGroupTotal) {
            return groupIdx;
        }
    }

    // Fallback if no group selected (could happen due to rounding errors)
    for (const auto &[groupIdx, samples]: groupedValidSamples) {
        if (groupIdx >= 0 && !samples.empty()) {
            return groupIdx;
        }
    }

    return -1;
}

int SampleManager::selectSampleFromGroup(const std::vector<int> &samplesInGroup) {
    // Calculate total probability for samples in this group
    float totalSampleProbability = 0.0f;
    for (int idx: samplesInGroup) {
        totalSampleProbability += getSampleProbability(idx);
    }

    // Select based on sample probability
    float randomSampleValue = juce::Random::getSystemRandom().nextFloat() * totalSampleProbability;
    float runningSampleTotal = 0.0f;

    for (int idx: samplesInGroup) {
        runningSampleTotal += getSampleProbability(idx);
        if (randomSampleValue <= runningSampleTotal) {
            return idx;
        }
    }

    // Fallback in case of rounding errors
    return samplesInGroup.size() - 1;
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
    return juce::File();
}

void SampleManager::createGroup(const juce::Array<int> &sampleIndices) {
    if (sampleIndices.isEmpty() || groups.size() >= 4)
        return;

    // Create a new group with auto-generated name
    juce::String groupName = "Group " + juce::String(groups.size() + 1);
    int groupIndex = groups.size();

    auto newGroup = std::make_unique<Group>(groupName, groupIndex);

    // Add sample indices to the group
    for (int i = 0; i < sampleIndices.size(); ++i) {
        int idx = sampleIndices[i];
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
    for (size_t i = 0; i < sampleList.size(); ++i) {
        if (sampleList[i]->groupIndex == groupIndex) {
            sampleList[i]->groupIndex = -1;
            // Also update the SamplerSound
            if (sampleList[i]->sound != nullptr)
                sampleList[i]->sound->setGroupIndex(-1);
        }
    }

    // Remove the group
    groups.erase(groups.begin() + groupIndex);

    // Update remaining group indices
    for (size_t i = groupIndex; i < groups.size(); ++i) {
        groups[i]->index = i;

        // Update sample group indices
        for (size_t j = 0; j < sampleList.size(); ++j) {
            if (sampleList[j]->groupIndex == i + 1) {
                sampleList[j]->groupIndex = i;
                // Also update the SamplerSound
                if (sampleList[j]->sound != nullptr)
                    sampleList[j]->sound->setGroupIndex(i);
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

void SampleManager::setSampleRateEnabled(int sampleIndex, Params::RateOption rate, bool enabled) {
    if (sampleIndex >= 0 && sampleIndex < sampleList.size()) {
        auto &sample = sampleList[sampleIndex];
        switch (rate) {
            case Params::RATE_1_1:
                sample->rate_1_1_enabled = enabled;
                break;
            case Params::RATE_1_2:
                sample->rate_1_2_enabled = enabled;
                break;
            case Params::RATE_1_4:
                sample->rate_1_4_enabled = enabled;
                break;
            case Params::RATE_1_8:
                sample->rate_1_8_enabled = enabled;
                break;
            case Params::RATE_1_16:
                sample->rate_1_16_enabled = enabled;
                break;
            case Params::RATE_1_32:
                sample->rate_1_32_enabled = enabled;
                break;
            default:
                break;
        }

        // Update the valid samples list for this rate
        updateValidSamplesForRate(rate);
    }
}

bool SampleManager::isSampleRateEnabled(int sampleIndex, Params::RateOption rate) const {
    if (sampleIndex >= 0 && sampleIndex < sampleList.size()) {
        const auto &sample = sampleList[sampleIndex];

        // Get the sample's rate state
        bool sampleRateEnabled = false;
        switch (rate) {
            case Params::RATE_1_1:
                sampleRateEnabled = sample->rate_1_1_enabled;
                break;
            case Params::RATE_1_2:
                sampleRateEnabled = sample->rate_1_2_enabled;
                break;
            case Params::RATE_1_4:
                sampleRateEnabled = sample->rate_1_4_enabled;
                break;
            case Params::RATE_1_8:
                sampleRateEnabled = sample->rate_1_8_enabled;
                break;
            case Params::RATE_1_16:
                sampleRateEnabled = sample->rate_1_16_enabled;
                break;
            case Params::RATE_1_32:
                sampleRateEnabled = sample->rate_1_32_enabled;
                break;

            default:
                return false;
        }

        // Return false early if sample itself has the rate disabled
        if (!sampleRateEnabled) return false;

        // Check if sample is in a valid group
        if (sample->groupIndex >= 0 && sample->groupIndex < groups.size()) {
            return isGroupRateEnabled(sample->groupIndex, rate);
        }

        // If no group or invalid group, and the sample has the rate enabled
        return true;
    }
    return false;
}

void SampleManager::updateValidSamplesForRate(Params::RateOption rate) {
    std::vector<int> *targetList = nullptr;
    switch (rate) {
        case Params::RATE_1_1:
            targetList = &validSamples_1_1;
            break;
        case Params::RATE_1_2:
            targetList = &validSamples_1_2;
            break;
        case Params::RATE_1_4:
            targetList = &validSamples_1_4;
            break;
        case Params::RATE_1_8:
            targetList = &validSamples_1_8;
            break;
        case Params::RATE_1_16:
            targetList = &validSamples_1_16;
            break;
        case Params::RATE_1_32:
            targetList = &validSamples_1_32;
            break;
        default:
            return;
    }

    targetList->clear();
    for (size_t i = 0; i < sampleList.size(); ++i) {
        if (isSampleRateEnabled(i, rate)) {
            targetList->push_back(i);
        }
    }
}

const std::vector<int> &SampleManager::getValidSamplesForRate(Params::RateOption rate) const {
    switch (rate) {
        case Params::RATE_1_1:
            return validSamples_1_1;
        case Params::RATE_1_2:
            return validSamples_1_2;
        case Params::RATE_1_4:
            return validSamples_1_4;
        case Params::RATE_1_8:
            return validSamples_1_8;
        case Params::RATE_1_16:
            return validSamples_1_16;
        case Params::RATE_1_32:
            return validSamples_1_32;
        default:
            return validSamples_1_4; // Default to quarter notes
    }
}

// Group rate methods implementation
void SampleManager::setGroupRateEnabled(int groupIndex, Params::RateOption rate, bool enabled) {
    if (groupIndex >= 0 && groupIndex < groups.size()) {
        auto &group = groups[groupIndex];
        switch (rate) {
            case Params::RATE_1_1:
                group->rate_1_1_enabled = enabled;
                break;
            case Params::RATE_1_2:
                group->rate_1_2_enabled = enabled;
                break;
            case Params::RATE_1_4:
                group->rate_1_4_enabled = enabled;
                break;
            case Params::RATE_1_8:
                group->rate_1_8_enabled = enabled;
                break;
            case Params::RATE_1_16:
                group->rate_1_16_enabled = enabled;
                break;
            case Params::RATE_1_32:
                group->rate_1_32_enabled = enabled;
                break;
            default:
                break;
        }

        updateValidSamplesForRate(rate);
    }
}

bool SampleManager::isGroupRateEnabled(int groupIndex, Params::RateOption rate) const {
    if (groupIndex >= 0 && groupIndex < groups.size()) {
        const auto &group = groups[groupIndex];
        switch (rate) {
            case Params::RATE_1_1:
                return group->rate_1_1_enabled;
            case Params::RATE_1_2:
                return group->rate_1_2_enabled;
            case Params::RATE_1_4:
                return group->rate_1_4_enabled;
            case Params::RATE_1_8:
                return group->rate_1_8_enabled;
            case Params::RATE_1_16:
                return group->rate_1_16_enabled;
            case Params::RATE_1_32:
                return group->rate_1_32_enabled;
            default:
                break;
        }
    }
    return false;
}

// Group effect methods implementation
void SampleManager::setGroupEffectEnabled(int groupIndex, int effectType, bool enabled) {
    if (groupIndex >= 0 && groupIndex < groups.size()) {
        auto &group = groups[groupIndex];
        switch (effectType) {
            case 0:
                group->reverb_enabled = enabled;
                break;  // Reverb
            case 1:
                group->stutter_enabled = enabled;
                break; // Stutter
            case 2:
                group->delay_enabled = enabled;
                break;   // Delay
            default:
                break;
        }
    }
}

bool SampleManager::isGroupEffectEnabled(int groupIndex, int effectType) const {
    if (groupIndex >= 0 && groupIndex < groups.size()) {
        const auto &group = groups[groupIndex];
        switch (effectType) {
            case 0:
                return group->reverb_enabled;  // Reverb
            case 1:
                return group->stutter_enabled; // Stutter
            case 2:
                return group->delay_enabled;   // Delay
            default:
                break;
        }
    }
    return false;
}

void SampleManager::normalizeSamples() {
    // If no samples loaded, nothing to normalize
    if (sampleList.empty()) {
        return;
    }

    // First, find the peak amplitude across all samples
    float globalPeak = 0.0f;

    // Find peak level for each sample
    for (const auto &sample: sampleList) {
        if (sample->sound) {
            juce::AudioBuffer<float> *audioData = sample->sound->getAudioData();
            if (audioData) {
                // Find the peak for this sample
                float samplePeak = 0.0f;

                // Check all channels
                for (int channel = 0; channel < audioData->getNumChannels(); ++channel) {
                    // Get peak for this channel
                    float channelPeak = audioData->getMagnitude(channel, 0, audioData->getNumSamples());
                    samplePeak = std::max(samplePeak, channelPeak);
                }

                // Update global peak if this sample has a higher peak
                globalPeak = std::max(globalPeak, samplePeak);
            }
        }
    }

    // If no audio data or all samples are silent, nothing to normalize
    if (globalPeak <= 0.0f) {
        return;
    }

    // Target level - typically 0.95 (-0.5dB) is a good value to prevent clipping
    const float targetLevel = 0.95f;

    // Normalize each sample to the target level
    for (auto &sample: sampleList) {
        if (sample->sound) {
            juce::AudioBuffer<float> *audioData = sample->sound->getAudioData();
            if (audioData) {
                // Find the peak for this sample
                float samplePeak = 0.0f;

                // Check all channels
                for (int channel = 0; channel < audioData->getNumChannels(); ++channel) {
                    // Get peak for this channel
                    float channelPeak = audioData->getMagnitude(channel, 0, audioData->getNumSamples());
                    samplePeak = std::max(samplePeak, channelPeak);
                }

                // Calculate gain to apply for normalization
                // We want the target level to equal the ratio of this sample's peak to the global peak
                float normalizationGain = (samplePeak > 0.0f) ? (targetLevel / globalPeak) : 1.0f;

                // Apply the gain to all channels
                for (int channel = 0; channel < audioData->getNumChannels(); ++channel) {
                    audioData->applyGain(channel, 0, audioData->getNumSamples(), normalizationGain);
                }
            }
        }
    }
}