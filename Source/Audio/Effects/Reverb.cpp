#include "Reverb.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <random>

Reverb::Reverb(std::shared_ptr<TimingManager> t)
    : timingManager(t)
{
    // Initialize JUCE reverb parameters with good default values
    juceReverbParams.roomSize = 0.8f;      // Large room size
    juceReverbParams.damping = 0.5f;       // Moderate damping
    juceReverbParams.wetLevel = 1.0f;      // Full wet signal
    juceReverbParams.dryLevel = 1.0f;      // Full dry signal (we'll handle mix ourselves)
    juceReverbParams.width = 1.0f;         // Full stereo width
    juceReverbParams.freezeMode = 0.0f;    // No freeze
    
    juceReverb.setParameters(juceReverbParams);
}

void Reverb::setSettings(Params::FxSettings s)
{
    settings = s;
    
    // Update JUCE reverb parameters based on our settings
    juceReverbParams.roomSize = settings.reverbTime / 100.0f;
    juceReverbParams.damping = settings.reverbDamping / 100.0f;
    juceReverbParams.width = settings.reverbWidth / 100.0f;
    juceReverb.setParameters(juceReverbParams);
}

void Reverb::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    this->sampleRate = sampleRate;
    this->currentBufferSize = samplesPerBlock;
    
    // Prepare JUCE reverb
    juceReverb.setSampleRate(sampleRate);
}

void Reverb::releaseResources()
{
    // Nothing to release for JUCE reverb
}

void Reverb::applyReverbEffect(juce::AudioBuffer<float>& buffer, 
                             const std::vector<int>& triggerSamplePositions)
{
    // Update the active notes based on MIDI trigger positions
    updateActiveNotes(triggerSamplePositions);
    
    // Check if we should apply reverb based on probability
    if (shouldApplyReverb())
    {
        // Create a temporary buffer for the wet signal
        juce::AudioBuffer<float> reverbBuffer;
        reverbBuffer.makeCopyOf(buffer);
        
        // Process with JUCE's reverb
        if (reverbBuffer.getNumChannels() == 2)
        {
            juceReverb.processStereo(reverbBuffer.getWritePointer(0),
                                   reverbBuffer.getWritePointer(1),
                                   reverbBuffer.getNumSamples());
        }
        else
        {
            juceReverb.processMono(reverbBuffer.getWritePointer(0),
                                 reverbBuffer.getNumSamples());
        }
        
        float wetMix = settings.reverbMix / 100.0f;
        
        if (settings.reverbProbability >= 99.9f)
        {
            // Apply to entire buffer with proper gain preservation
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                auto* dry = buffer.getWritePointer(channel);
                auto* wet = reverbBuffer.getReadPointer(channel);
                
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                {
                    // Preserve gain by using equal-power crossfade
                    float dryGain = std::cos(wetMix * juce::MathConstants<float>::halfPi);
                    float wetGain = std::sin(wetMix * juce::MathConstants<float>::halfPi);
                    dry[sample] = dry[sample] * dryGain + wet[sample] * wetGain;
                }
            }
        }
        else if (!triggerSamplePositions.empty())
        {
            // Apply only to the first trigger position in this buffer
            int startSample = triggerSamplePositions[0];
            if (startSample >= 0 && startSample < buffer.getNumSamples())
            {
                // Get note duration
                double bpm = timingManager->getBpm();
                int noteDuration = bpm > 0.0 
                    ? static_cast<int>(60.0 / bpm * sampleRate)  // One quarter note
                    : static_cast<int>(sampleRate * 0.5);        // 500ms fallback
                
                int endSample = juce::jmin(buffer.getNumSamples(), startSample + noteDuration);
                
                // Apply reverb with proper gain preservation
                for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                {
                    auto* dry = buffer.getWritePointer(channel);
                    auto* wet = reverbBuffer.getReadPointer(channel);
                    
                    for (int sample = startSample; sample < endSample; ++sample)
                    {
                        float dryGain = std::cos(wetMix * juce::MathConstants<float>::halfPi);
                        float wetGain = std::sin(wetMix * juce::MathConstants<float>::halfPi);
                        dry[sample] = dry[sample] * dryGain + wet[sample] * wetGain;
                    }
                }
            }
        }
    }
}

bool Reverb::shouldApplyReverb()
{
    // Determine whether to apply reverb based on probability setting
    return settings.reverbProbability > 0.0f &&
           (settings.reverbProbability >= 100.0f || 
            (static_cast<float>(rand()) / RAND_MAX) * 100.0f <= settings.reverbProbability);
}

void Reverb::updateActiveNotes(const std::vector<int>& triggerSamplePositions)
{
    // Process and remove completed notes
    for (auto it = activeNotes.begin(); it != activeNotes.end();)
    {
        it->duration -= currentBufferSize;
        if (it->duration <= 0)
        {
            it = activeNotes.erase(it);
        }
        else
        {
            ++it;
        }
    }
    
    // Add new notes from trigger positions
    for (auto pos : triggerSamplePositions)
    {
        // Get a note duration based on current tempo and time signature
        int noteDuration = 0;
        
        // Get timing info directly from the timing manager
        double bpm = timingManager->getBpm();
        bool isPlaying = bpm > 0.0; // If BPM is available, we're likely playing
        
        if (isPlaying)
        {
            // Default to quarter note duration, can be made more sophisticated
            double quarterNoteInSamples = 60.0 / bpm * sampleRate;
            noteDuration = static_cast<int>(quarterNoteInSamples);
        }
        else
        {
            // Fallback duration if timing info not available
            noteDuration = static_cast<int>(sampleRate * 0.5); // 500ms
        }
        
        activeNotes.push_back({pos, noteDuration, true});
    }
}
