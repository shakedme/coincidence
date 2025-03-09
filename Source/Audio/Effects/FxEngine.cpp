#include <juce_audio_utils/juce_audio_utils.h>
#include "../Shared/TimingManager.h"
#include "FxEngine.h"
#include "../PluginProcessor.h"

FxEngine::FxEngine(std::shared_ptr<TimingManager> t, PluginProcessor &processorRef)
        : timingManager(t), processor(processorRef) {
    // Get a reference to the SampleManager
    auto& sampleManager = processor.getSampleManager();
    
    // Create effects with the SampleManager reference
    stutterEffect = std::make_unique<Stutter>(timingManager, sampleManager);
    reverbEffect = std::make_unique<Reverb>(timingManager, sampleManager);
    delayEffect = std::make_unique<Delay>(timingManager, sampleManager);
}

FxEngine::~FxEngine() {
    releaseResources();
}

void FxEngine::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Store audio settings
    this->sampleRate = sampleRate;
    this->bufferSize = samplesPerBlock;

    // init FX
    reverbEffect->prepareToPlay(sampleRate, samplesPerBlock);
    stutterEffect->prepareToPlay(sampleRate, samplesPerBlock);
    delayEffect->prepareToPlay(sampleRate, samplesPerBlock);
}

void FxEngine::releaseResources() {
    reverbEffect->releaseResources();
    stutterEffect->releaseResources();
    delayEffect->releaseResources();
}

void FxEngine::setSettings(Params::FxSettings _settings) {
    settings = _settings;
    reverbEffect->setSettings(_settings);
    stutterEffect->setSettings(_settings);
    delayEffect->setSettings(_settings);
}

void FxEngine::processAudio(juce::AudioBuffer<float> &buffer,
                            juce::AudioPlayHead *playHead,
                            const juce::MidiBuffer &midiMessages) {
    updateTimingInfo(playHead);
    std::vector<juce::int64> triggerPositions = checkForMidiTriggers(midiMessages);
    std::vector<juce::int64> noteDurations = getNoteDurations(triggerPositions);

    // Apply effects in order: reverb, delay, stutter
    reverbEffect->applyReverbEffect(buffer, triggerPositions, noteDurations);
    delayEffect->applyDelayEffect(buffer, triggerPositions, noteDurations);
    stutterEffect->applyStutterEffect(buffer, triggerPositions);
}

std::vector<juce::int64> FxEngine::getNoteDurations(const std::vector<juce::int64> &triggerPositions) {
    // Get the pending notes from the NoteGenerator
    const auto &pendingNotes = processor.getNoteGenerator().getPendingNotes();
    const auto &noteGenerator = processor.getNoteGenerator();

    std::vector<juce::int64> noteDurations;

    // For each trigger position, find the corresponding note duration
    for (juce::int64 triggerPos: triggerPositions) {
        bool foundDuration = false;

        // First check pending notes
        for (const auto &note: pendingNotes) {
            if (note.startSamplePosition == triggerPos) {
                noteDurations.push_back(note.durationInSamples);
                foundDuration = true;
                break;
            }
        }

        // If not found in pending notes, check if it's the current active note
        if (!foundDuration && noteGenerator.isNoteActive()) {
            // If this trigger position matches the current note's start position
            if (triggerPos
                == 0) // Current note starts at the beginning of this buffer
            {
                noteDurations.push_back(noteGenerator.getCurrentNoteDuration());
                foundDuration = true;
            }
        }

        // If still no duration found, use a default duration
        if (!foundDuration) {
            noteDurations.push_back(
                    static_cast<int>(sampleRate * 0.5)); // 500ms fallback
        }
    }

    return noteDurations;
}

void FxEngine::updateTimingInfo(juce::AudioPlayHead *playHead) {
    if (playHead != nullptr) {
        timingManager->updateTimingInfo(playHead);
    }
}

std::vector<juce::int64>
FxEngine::checkForMidiTriggers(const juce::MidiBuffer &midiMessages) {
    std::vector<juce::int64> triggerPositions;

    // Look for MIDI note-on events to use as reference points
    if (!midiMessages.isEmpty()) {
        for (const auto metadata: midiMessages) {
            auto message = metadata.getMessage();
            if (message.isNoteOn()) {
                // Found a note-on - this is a good point to start effects
                triggerPositions.push_back(metadata.samplePosition);
            }
        }
    }

    return triggerPositions;
}
