#include "JammerAudioProcessor.h"
#include "PluginProcessor.h"
#include "Sampler.h"

JammerAudioProcessor::JammerAudioProcessor(PluginProcessor& processor)
    : processor(processor)
{
}

void JammerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sampleManager.prepareToPlay(sampleRate);
    
    // Initialize our buffer counter for voice cleanup
    bufferCounter = 0;
}

void JammerAudioProcessor::releaseResources()
{
    // Make sure to clean up all notes when resources are released
    sampleManager.getSampler().allNotesOff(0, true);
}

void JammerAudioProcessor::processAudio(juce::AudioBuffer<float>& buffer,
                                 juce::MidiBuffer& processedMidi,
                                 juce::MidiBuffer& midiMessages)
{
    // If we have samples loaded, process the MIDI through our sampler
    if (sampleManager.isSampleLoaded())
    {
        // Increment our counter and occasionally do a full voice cleanup
        // to prevent voice leaks that could cause dropouts
        bufferCounter++;
        if (bufferCounter >= 1000) { // About every few seconds depending on buffer size
            sampleManager.getSampler().allNotesOff(0, false);
            bufferCounter = 0;
        }
        
        // Get the current active sample index from the note generator
        int currentSampleIdx = processor.getNoteGenerator().getCurrentActiveSampleIdx();
        
        // Log current sample for debugging
        // Only log when the sample index changes to avoid excessive logging
        static int lastLoggedSampleIdx = -1;
        if (currentSampleIdx != lastLoggedSampleIdx) {
            juce::String message = "Current sample index: " + juce::String(currentSampleIdx) + 
                                   " (out of " + juce::String(sampleManager.getNumSamples()) + " samples)";
            juce::Logger::writeToLog(message);
            lastLoggedSampleIdx = currentSampleIdx;
        }
        
        // Set the global sample index directly
        if (currentSampleIdx >= 0 && currentSampleIdx < sampleManager.getNumSamples()) {
            // Update the global index that all voices will check
            SamplerVoice::setCurrentSampleIndex(currentSampleIdx);
            
            // If the sample index has changed, we need to stop any active notes
            // to make sure we use the new sample for upcoming notes
            static int lastPlayedSampleIdx = -1;
            if (currentSampleIdx != lastPlayedSampleIdx) {
                // Stop all notes but don't reset voices completely
                // This allows for quicker sample switching without audio dropouts
                sampleManager.getSampler().allNotesOff(0, false);
                
                lastPlayedSampleIdx = currentSampleIdx;
                juce::Logger::writeToLog("Sample index changed - stopping active notes");
            }
        }
        
        // Create a new MIDI buffer with modified messages that include the sample index
        juce::MidiBuffer modifiedMidi;
        
        // Only process if we have a valid sample index
        if (currentSampleIdx >= 0 && currentSampleIdx < sampleManager.getNumSamples()) {
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
                        juce::String debugMsg = "Sending controller for sample: " + juce::String(currentSampleIdx);
                        juce::Logger::writeToLog(debugMsg);
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
        sampleManager.getSampler().renderNextBlock(
            buffer, modifiedMidi, 0, buffer.getNumSamples());

        // Now the buffer contains the synthesized audio
        // We clear the MIDI buffer since the sampler has processed it
        processedMidi.clear();
    }
    else
    {
        // If no samples are loaded, pass through our generated MIDI
        midiMessages.swapWith(processedMidi);
    }
} 