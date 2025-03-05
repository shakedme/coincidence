#include "Reverb.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <random>

Reverb::Reverb(std::shared_ptr<TimingManager> t)
    : timingManager(t)
    , feedbackMatrix(NUM_DELAY_LINES, NUM_DELAY_LINES)
{
    // Initialize the FDN
    initializeFDN();
}

void Reverb::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    this->sampleRate = sampleRate;
    this->currentBufferSize = samplesPerBlock;
    
    // Allocate work buffer
    workBuffer.setSize(2, samplesPerBlock);
    
    // Reset delay lines and filters
    clearDelayLines();
    
    // Initialize delay lengths with prime numbers for better diffusion
    int primeLengths[NUM_DELAY_LINES] = {743, 811, 877, 947, 1013, 1087, 1153, 1223};
    
    // Adjust delay lengths based on sample rate
    for (int i = 0; i < NUM_DELAY_LINES; ++i)
    {
        // Scale delay lengths for current sample rate
        delayLengths[i] = static_cast<int>(primeLengths[i] * sampleRate / 44100.0);
        
        // Resize delay lines
        delayLines[i].resize(delayLengths[i], 0.0f);
        writePositions[i] = 0;
        
        // Initialize filters
        juce::dsp::IIR::Coefficients<float>::Ptr coeffs = 
            juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 8000.0f);
        lowpassFilters[i].coefficients = coeffs;
    }
}

void Reverb::releaseResources()
{
    // Clear all buffers when not in use
    for (auto& delayLine : delayLines)
        delayLine.clear();
    
    workBuffer.clear();
}

void Reverb::clearDelayLines()
{
    // Reset all delay lines to zero
    for (auto& delayLine : delayLines)
        std::fill(delayLine.begin(), delayLine.end(), 0.0f);
    
    // Reset write positions
    for (auto& pos : writePositions)
        pos = 0;
}

void Reverb::initializeFDN()
{
    // Build a Hadamard matrix for good mixing properties
    // Start with basic 2x2 Hadamard matrix
    float hadamard2[2][2] = {
        {1.0f,  1.0f},
        {1.0f, -1.0f}
    };
    
    // Expand to 8x8 using Kronecker product
    float normalizationFactor = 1.0f / std::sqrt(static_cast<float>(NUM_DELAY_LINES));
    
    for (int i = 0; i < NUM_DELAY_LINES; ++i)
    {
        for (int j = 0; j < NUM_DELAY_LINES; ++j)
        {
            // Compute Hadamard matrix values using bit operations 
            // (counts the number of matching 1 bits in i and j)
            int bitCount = 0;
            for (int k = 0; k < 3; ++k) // 3 bits for 8x8
                if ((i & (1 << k)) && (j & (1 << k)))
                    ++bitCount;
            
            // Apply normalization factor 1/sqrt(N)
            feedbackMatrix(i, j) = (bitCount % 2 == 0 ? 1.0f : -1.0f) * normalizationFactor;
        }
    }
}

void Reverb::applyReverbEffect(juce::AudioBuffer<float>& buffer, 
                             const std::vector<int>& triggerSamplePositions)
{
    // Update the active notes based on MIDI trigger positions
    updateActiveNotes(triggerSamplePositions);
    
    // Check if we should apply reverb based on probability
    if (shouldApplyReverb())
    {
        // If probability is 100%, apply to entire buffer
        if (settings.reverbProbability >= 99.9f)
        {
            processFDN(buffer);
        }
        else
        {
            // Otherwise, apply selectively based on active notes
            applySelectiveReverb(buffer, triggerSamplePositions);
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

void Reverb::applySelectiveReverb(juce::AudioBuffer<float>& buffer, 
                                const std::vector<int>& triggerSamplePositions)
{
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    
    // Create a temporary buffer for the processed reverb
    juce::AudioBuffer<float> reverbBuffer;
    reverbBuffer.makeCopyOf(buffer);
    
    // Process the reverb on the temporary buffer
    processFDN(reverbBuffer);
    
    // Create a mix buffer for gradual cross-fading
    juce::AudioBuffer<float> mixBuffer;
    mixBuffer.setSize(numChannels, numSamples);
    mixBuffer.clear();
    
    // Apply wet signal only for active note regions
    for (const auto& note : activeNotes)
    {
        // Calculate the region within this buffer
        int startSample = note.startSample;
        int endSample = std::min(numSamples, startSample + note.duration);
        
        if (startSample < 0)
        {
            startSample = 0;
        }
        
        if (startSample < numSamples && endSample > 0)
        {
            // Copy the reverb signal to the mix buffer for this region
            for (int channel = 0; channel < numChannels; ++channel)
            {
                for (int sample = startSample; sample < endSample; ++sample)
                {
                    mixBuffer.setSample(channel, sample, reverbBuffer.getSample(channel, sample));
                }
            }
        }
    }
    
    // Apply wet/dry mix
    float wetMix = settings.reverbMix / 100.0f;
    
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* dryData = buffer.getWritePointer(channel);
        const auto* wetData = mixBuffer.getReadPointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            dryData[sample] = dryData[sample] * (1.0f - wetMix) + wetData[sample] * wetMix;
        }
    }
}

void Reverb::processFDN(juce::AudioBuffer<float>& buffer)
{
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    
    // Copy input to work buffer
    workBuffer.makeCopyOf(buffer);
    
    // Apply wet/dry mix
    float wetMix = settings.reverbMix / 100.0f;
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Read output from delay lines
        std::array<float, NUM_DELAY_LINES> delayOutputs;
        for (int i = 0; i < NUM_DELAY_LINES; ++i)
        {
            int readPos = (writePositions[i] - 1 + delayLengths[i]) % delayLengths[i];
            delayOutputs[i] = delayLines[i][readPos];
        }
        
        // Mix delay outputs through feedback matrix
        std::array<float, NUM_DELAY_LINES> mixedOutputs;
        for (int i = 0; i < NUM_DELAY_LINES; ++i)
        {
            mixedOutputs[i] = 0.0f;
            for (int j = 0; j < NUM_DELAY_LINES; ++j)
            {
                mixedOutputs[i] += delayOutputs[j] * feedbackMatrix(i, j);
            }
        }
        
        // Calculate RT60 decay factor
        float decayFactor = std::pow(0.001f, 1.0f / (sampleRate * MAX_REVERB_TIME * (settings.reverbTime / 100.0f)));
        
        // Get input from all channels (summed)
        float inputSample = 0.0f;
        for (int channel = 0; channel < numChannels; ++channel)
        {
            inputSample += workBuffer.getSample(channel, sample) / static_cast<float>(numChannels);
        }
        
        // Process each delay line with input distribution
        for (int i = 0; i < NUM_DELAY_LINES; ++i)
        {
            // Apply filter
            float filtered = lowpassFilters[i].processSample(mixedOutputs[i] * decayFactor);
            
            // Write to delay line: input + feedback
            delayLines[i][writePositions[i]] = inputSample * 0.25f + filtered;
            
            // Update write position
            writePositions[i] = (writePositions[i] + 1) % delayLengths[i];
        }
        
        // Mix output back to buffer with wet/dry balance
        float outputL = (delayOutputs[0] + delayOutputs[2] + delayOutputs[4] + delayOutputs[6]) * 0.25f;
        float outputR = (delayOutputs[1] + delayOutputs[3] + delayOutputs[5] + delayOutputs[7]) * 0.25f;
        
        // Apply the output to channels
        if (numChannels >= 2)
        {
            // Stereo output
            float dryL = buffer.getSample(0, sample);
            float dryR = buffer.getSample(1, sample);
            
            buffer.setSample(0, sample, dryL * (1.0f - wetMix) + outputL * wetMix);
            buffer.setSample(1, sample, dryR * (1.0f - wetMix) + outputR * wetMix);
        }
        else
        {
            // Mono output
            float dry = buffer.getSample(0, sample);
            buffer.setSample(0, sample, dry * (1.0f - wetMix) + outputL * wetMix);
        }
    }
}
