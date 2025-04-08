#include "NoteGenerator.h"
#include "../../Gui/PluginEditor.h"

NoteGenerator::NoteGenerator(PluginProcessor &processorRef)
        : processor(processorRef),
          timingManager(processorRef.getTimingManager()) {
    releaseResources();

    scaleManager = std::make_unique<ScaleManager>(processor);

    std::vector<StructParameter<Models::MidiSettings>::FieldDescriptor> descriptors = {
            makeFieldDescriptor(Params::ID_RHYTHM_1_1, &Models::MidiSettings::barProbability),
            makeFieldDescriptor(Params::ID_RHYTHM_1_2, &Models::MidiSettings::halfBarProbability),
            makeFieldDescriptor(Params::ID_RHYTHM_1_4, &Models::MidiSettings::quarterBarProbability),
            makeFieldDescriptor(Params::ID_RHYTHM_1_8, &Models::MidiSettings::eighthBarProbability),
            makeFieldDescriptor(Params::ID_RHYTHM_1_16, &Models::MidiSettings::sixteenthBarProbability),
            makeFieldDescriptor(Params::ID_RHYTHM_1_32, &Models::MidiSettings::thirtySecondBarProbability),
            makeFieldDescriptor(Params::ID_PROBABILITY, &Models::MidiSettings::probability),
            makeFieldDescriptor(Params::ID_GATE, &Models::MidiSettings::gateValue),
            makeFieldDescriptor(Params::ID_GATE_RANDOMIZE, &Models::MidiSettings::gateRandomize),
            makeFieldDescriptor(Params::ID_GATE_DIRECTION, &Models::MidiSettings::gateDirection),
            makeFieldDescriptor(Params::ID_VELOCITY, &Models::MidiSettings::velocityValue),
            makeFieldDescriptor(Params::ID_VELOCITY_RANDOMIZE, &Models::MidiSettings::velocityRandomize),
            makeFieldDescriptor(Params::ID_VELOCITY_DIRECTION, &Models::MidiSettings::velocityDirection)
    };

    settingsBinding = std::make_unique<StructParameter<Models::MidiSettings>>(
            processor.getModulationMatrix(), descriptors);
}

void NoteGenerator::prepareToPlay(double sampleRate, int) {
    releaseResources();
}

void NoteGenerator::releaseResources() {
    // Clear any active notes
    noteIsActive = false;
    isInputNoteActive = false;
    currentInputNote = -1;
    currentActiveNote = -1;
    currentActiveSampleIdx = -1;

    // Clear pending notes
    pendingNotes.clear();
}

void NoteGenerator::processIncomingMidi(const juce::MidiBuffer &midiMessages,
                                        juce::MidiBuffer &processedMidi,
                                        int numSamples
) {
    settings = settingsBinding->getValue();

    for (const auto metadata: midiMessages) {
        auto message = metadata.getMessage();
        const int time = metadata.samplePosition;

        if (message.isNoteOn()) {
            currentInputNote = message.getNoteNumber();
            isInputNoteActive = true;
        } else if (message.isNoteOff() && message.getNoteNumber() == currentInputNote) {
            isInputNoteActive = false;
            if (noteIsActive && currentActiveNote >= 0) {
                stopActiveNote(processedMidi, time);
            }
        } else if (!message.isNoteOnOrOff()) {
            processedMidi.addEvent(message, time);
        }
    }

    // Check if active notes need to be turned off
    checkActiveNotes(processedMidi, numSamples);

    // Process any pending notes scheduled from previous buffers
    processPendingNotes(processedMidi, numSamples);

    // Generates midi based on settings
    generateNewNotes(processedMidi);
}

void NoteGenerator::checkActiveNotes(juce::MidiBuffer &midiMessages, int numSamples) {
    if (noteIsActive) // Only check if we have an active note
    {
        // Calculate when the note should end (in samples relative to the start of this buffer)
        juce::int64 noteEndPosition = (noteStartPosition + noteDurationInSamples)
                                      - timingManager.getSamplePosition();

        // If the note should end during this buffer
        if (noteEndPosition >= 0 && noteEndPosition < numSamples) {
            // Send note off at the exact sample position it should end
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentActiveNote),
                                  static_cast<int>(noteEndPosition));
            noteIsActive = false;
            currentActiveNote = -1;
            currentActiveSampleIdx = -1;
        }
    }
}

std::vector<NoteGenerator::EligibleRate> NoteGenerator::collectEligibleRates(float &totalWeight) {
    std::vector<EligibleRate> eligibleRates;
    totalWeight = 0.0f;

    // Collect all rates that should trigger at this position
    for (int rateIndex = 0; rateIndex < Models::NUM_RATE_OPTIONS; ++rateIndex) {
        auto rate = static_cast<Models::RateOption>(rateIndex);
        auto rateValue = settings.getRateValueByIdx(rateIndex);
        // Only consider rates with non-zero value
        if (rateValue > 0.0f) {
            // Check if we should trigger a note at this rate
            if (processor.getTimingManager().shouldTriggerNote(rate)) {
                eligibleRates.push_back({rate, rateValue});
                totalWeight += rateValue;
            }
        }
    }

    return eligibleRates;
}

Models::RateOption NoteGenerator::selectRateFromEligible(const std::vector<EligibleRate> &eligibleRates,
                                                         float totalWeight) {
    // Safety check - if only one rate is eligible, use it
    if (eligibleRates.size() == 1)
        return eligibleRates[0].rate;

    // If no rates are eligible (shouldn't happen, but just in case)
    if (eligibleRates.empty())
        return Models::RATE_1_4; // Default to quarter notes

    // Select a rate based on weighted probability
    float randomValue = juce::Random::getSystemRandom().nextFloat() * totalWeight;
    float cumulativeWeight = 0.0f;

    for (const auto &rate: eligibleRates) {
        cumulativeWeight += rate.weight;
        if (randomValue <= cumulativeWeight)
            return rate.rate;
    }

    // Fallback in case of rounding errors
    return eligibleRates.back().rate;
}

void NoteGenerator::generateNewNotes(juce::MidiBuffer &midiMessages) {
    if (!isInputNoteActive || noteIsActive) {
        return;
    }

    float totalWeight = 0.0f;
    auto eligibleRates = collectEligibleRates(totalWeight);

    // Only proceed if we have eligible rates
    if (totalWeight > 0.0f && !eligibleRates.empty()) {
        // Determine if any note should play
        float triggerProbability = settings.probability;
        bool shouldPlayNote =
                juce::Random::getSystemRandom().nextFloat() < triggerProbability
                || settings.probability == 100.0f;

        if (shouldPlayNote) {
            // Select a rate based on weighted probability
            Models::RateOption selectedRate =
                    selectRateFromEligible(eligibleRates, totalWeight);

            // Generate and play a new note
            playNewNote(selectedRate, midiMessages);
        }
    }
}

void NoteGenerator::playNewNote(Models::RateOption selectedRate,
                                juce::MidiBuffer &midiMessages) {
    // Calculate next expected grid position
    double nextExpectedGridPoint = timingManager.getNextExpectedGridPoint(
            selectedRate, static_cast<int>(selectedRate));
    double ppqPosition = timingManager.getPpqPosition();
    double bpm = timingManager.getBpm();

    // Calculate precise sample position for this grid point
    double samplesPerQuarterNote = (60.0 / bpm) * timingManager.getSampleRate();
    double ppqOffsetFromCurrent = nextExpectedGridPoint - ppqPosition;

    // Convert to sample offset - this ensures grid alignment
    int sampleOffset = static_cast<int>(ppqOffsetFromCurrent * samplesPerQuarterNote);

    // If we somehow missed the grid point, play ASAP
    if (sampleOffset < 0) {
        sampleOffset = 0;
    }

    juce::int64 absoluteNotePosition = timingManager.getSamplePosition() + sampleOffset;

    // Calculate note properties
    int noteLengthSamples = calculateNoteLength(selectedRate);
    int noteToPlay = scaleManager->applyScaleAndModifications(currentInputNote);
    int velocity = calculateVelocity();

    // Determine which sample to use
    int sampleIndex = -1;
    if (processor.getSampleManager().isSampleLoaded()) {
        sampleIndex = processor.getSampleManager().getNextSampleIndex(selectedRate);
    }

    int bufferSize = processor.getBlockSize();

    if (sampleOffset < bufferSize) {
        // Immediate playback - note falls within current buffer
        addNoteWithinCurrentBuffer(midiMessages,
                                   noteToPlay,
                                   velocity,
                                   sampleOffset,
                                   absoluteNotePosition,
                                   noteLengthSamples,
                                   sampleIndex);
    } else {
        addPendingNote(
                noteToPlay, velocity, noteLengthSamples, sampleIndex, absoluteNotePosition);
    }

    // Update lastTriggerTimes to exactly the grid point we just played
    // This ensures the next note will be spaced exactly one grid interval away
    timingManager.updateLastTriggerTime(selectedRate, nextExpectedGridPoint);

    // If we were in a loop, we're now past that state
    timingManager.clearLoopDetection();
}

void NoteGenerator::addPendingNote(int noteToPlay,
                                   int velocity,
                                   int noteLengthSamples,
                                   int sampleIndex,
                                   juce::int64 absoluteNotePosition) {
    // Schedule for future buffer
    PendingNote pendingNote;
    pendingNote.noteNumber = noteToPlay;
    pendingNote.velocity = velocity;
    pendingNote.startSamplePosition = absoluteNotePosition;
    pendingNote.durationInSamples = noteLengthSamples;
    pendingNote.sampleIndex = sampleIndex;

    // Add to pending notes queue
    pendingNotes.push_back(pendingNote);
}

void NoteGenerator::addNoteWithinCurrentBuffer(juce::MidiBuffer &midiMessages,
                                               int noteToPlay,
                                               int velocity,
                                               int sampleOffset,
                                               juce::int64 absoluteNotePosition,
                                               int noteLengthSamples,
                                               int sampleIndex) {
    midiMessages.addEvent(
            juce::MidiMessage::noteOn(1, noteToPlay, (juce::uint8) velocity), sampleOffset);

    // Store the active note data
    currentActiveNote = noteToPlay;
    currentActiveVelocity = velocity;
    currentActiveSampleIdx = sampleIndex;

    noteStartPosition = absoluteNotePosition;
    noteDurationInSamples = noteLengthSamples;
    noteIsActive = true;

    // Update keyboard state
    if (auto *editor = dynamic_cast<PluginEditor *>(processor.getActiveEditor())) {
        editor->updateKeyboardState(true, currentActiveNote, currentActiveVelocity);
    }
}

void NoteGenerator::processPendingNotes(juce::MidiBuffer &midiMessages, int numSamples) {
    if (pendingNotes.empty())
        return;

    // Process the pending notes that fall within the current buffer
    auto it = pendingNotes.begin();
    while (it != pendingNotes.end()) {
        // Calculate local buffer position
        juce::int64 localPosition =
                it->startSamplePosition - timingManager.getSamplePosition();

        // If the note start position is in this buffer
        if (localPosition >= 0 && localPosition < numSamples) {
            // Add the note at the calculated position
            midiMessages.addEvent(
                    juce::MidiMessage::noteOn(1, it->noteNumber, (juce::uint8) it->velocity),
                    static_cast<int>(localPosition));

            // Update the active note info
            currentActiveNote = it->noteNumber;
            currentActiveVelocity = it->velocity;
            currentActiveSampleIdx = it->sampleIndex;
            noteStartPosition = it->startSamplePosition;
            noteDurationInSamples = it->durationInSamples;
            noteIsActive = true;

            // Update keyboard state
            if (auto *editor = dynamic_cast<PluginEditor *>(processor.getActiveEditor())) {
                editor->updateKeyboardState(
                        true, currentActiveNote, currentActiveVelocity);
            }

            // Remove the played note from pending list
            it = pendingNotes.erase(it);
        }
            // If the note start position is before this buffer, it's too late to play it
        else if (localPosition < 0) {
            it = pendingNotes.erase(it);
        } else {
            // Note is still in the future
            ++it;
        }
    }
}

int NoteGenerator::calculateNoteLength(Models::RateOption rate) {
    // Get base duration in samples for this rate
    double baseDuration = timingManager.getNoteDurationInSamples(rate);

    // Apply gate percentage (0-100%)
    double gateValue = settings.gateValue; // Convert to 0.0-1.0

    // Only apply randomization if it's actually enabled
    if (settings.gateRandomize > 0.0f) {
        gateValue = applyRandomization(
                settings.gateValue, settings.gateRandomize, settings.gateDirection);
        currentRandomizedGate = static_cast<float>(gateValue);
    }

    gateValue = juce::jlimit(0.1, 0.98, gateValue);

    // Calculate final note length in samples - use a precise calculation
    int lengthInSamples = static_cast<int>(baseDuration * gateValue);

    // Minimum length safety check - at least 5ms
    int minLengthSamples = static_cast<int>(timingManager.getSampleRate() * 0.005);
    return std::max(lengthInSamples, minLengthSamples);
}

int NoteGenerator::calculateVelocity() {
    double velocityValue = settings.velocityValue * 127.0;

    if (settings.velocityRandomize > 0.0f) {
        velocityValue = applyRandomization(settings.velocityValue,
                                           settings.velocityRandomize,
                                           settings.velocityDirection);
        currentRandomizedVelocity = static_cast<float>(velocityValue);
        velocityValue = velocityValue * 127.0f;
        velocityValue = juce::jlimit(1.0, 127.0, velocityValue);
    }

    return static_cast<int>(velocityValue);
}

float NoteGenerator::applyRandomization(float value,
                                        float randomizeValue,
                                        Models::DirectionType direction) const {
    float maxValue = juce::jmin(100.0f, value + randomizeValue);
    float minValue = juce::jmax(0.0f, value - randomizeValue);
    float rightValue =
            juce::jmap(juce::Random::getSystemRandom().nextFloat(), value, maxValue);
    float leftValue =
            juce::jmap(juce::Random::getSystemRandom().nextFloat(), minValue, value);

    if (direction == Models::DirectionType::RIGHT) {
        return rightValue;
    } else if (direction == Models::DirectionType::LEFT) {
        return leftValue;
    } else {
        return juce::Random::getSystemRandom().nextFloat() > 0.5 ? rightValue : leftValue;
    }
}

void NoteGenerator::stopActiveNote(juce::MidiBuffer &midiMessages,
                                   int currentSamplePosition) {
    if (noteIsActive && currentActiveNote >= 0) {
        // Send note off message - channel 1 (fixed)
        midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentActiveNote),
                              currentSamplePosition);

        // Update keyboard state
        if (auto *editor = dynamic_cast<PluginEditor *>(processor.getActiveEditor())) {
            editor->updateKeyboardState(false, currentActiveNote, 0);
        }

        noteIsActive = false;
        currentActiveNote = -1;
    }
}