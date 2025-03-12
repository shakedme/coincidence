#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../Config.h"

/**
 * Class to manage musical scales and note modifications
 */
class ScaleManager
{
public:
    ScaleManager();
    ~ScaleManager() = default;
    
    // Apply scale and modifications to a note according to the settings
    int applyScaleAndModifications(int noteNumber, const Config::GeneratorSettings& settings);
    
    // Check if a note is in the specified scale
    bool isNoteInScale(int note, const juce::Array<int>& scale);
    
    // Find the closest note in scale to the given note
    int findClosestNoteInScale(int note, const juce::Array<int>& scale, int root);

    // Get the scale array based on scale type
    juce::Array<int> getSelectedScale(Config::ScaleType scaleType);
    
    // Reset arpeggiator state
    void resetArpeggiator();
    
private:
    // Arpeggiator state
    int currentArpStep = 0;
    bool arpDirectionUp = true;
}; 