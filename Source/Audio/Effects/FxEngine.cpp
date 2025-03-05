// GlitchEngine.h modifications

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "../Shared/TimingManager.h"
#include "FxEngine.h"

FxEngine::FxEngine(std::shared_ptr<TimingManager> t)
    : timingManager(t)
{
    stutterEffect = std::make_unique<Stutter>(timingManager);
    reverbEffect = std::make_unique<Reverb>(timingManager);
}

FxEngine::~FxEngine()
{
    releaseResources();
}

void FxEngine::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Store audio settings
    this->sampleRate = sampleRate;
    this->bufferSize = samplesPerBlock;

    // init FX
    reverbEffect->prepareToPlay(sampleRate, samplesPerBlock);
    stutterEffect->prepareToPlay(sampleRate, samplesPerBlock);
}

void FxEngine::releaseResources()
{
    reverbEffect->releaseResources();
    stutterEffect->releaseResources();
}

void FxEngine::setSettings(Params::FxSettings s)
{
    settings = s;
    reverbEffect->setSettings(s);
    stutterEffect->setSettings(s);
}

void FxEngine::updateFxWithBufferSize(int numSamples)
{
    reverbEffect->setBufferSize(numSamples);
    stutterEffect->setBufferSize(numSamples);
}

void FxEngine::processAudio(juce::AudioBuffer<float>& buffer,
                            juce::AudioPlayHead* playHead,
                            const juce::MidiBuffer& midiMessages)
{
    updateTimingInfo(playHead);
    updateFxWithBufferSize(buffer.getNumSamples());

    std::vector<int> triggerPositions = checkForMidiTriggers(midiMessages);
    
    // Apply reverb BEFORE stutter effect
    reverbEffect->applyReverbEffect(buffer, triggerPositions);
    
    // Then apply stutter effect
    stutterEffect->applyStutterEffect(buffer, triggerPositions);
}

void FxEngine::updateTimingInfo(juce::AudioPlayHead* playHead)
{
    if (playHead != nullptr)
    {
        timingManager->updateTimingInfo(playHead);
    }
}

std::vector<int> FxEngine::checkForMidiTriggers(const juce::MidiBuffer& midiMessages)
{
    std::vector<int> triggerPositions;

    // Look for MIDI note-on events to use as reference points
    if (!midiMessages.isEmpty())
    {
        for (const auto metadata: midiMessages)
        {
            auto message = metadata.getMessage();
            if (message.isNoteOn())
            {
                // Found a note-on - this is a good point to start effects
                triggerPositions.push_back(metadata.samplePosition);
            }
        }
    }

    return triggerPositions;
}
