#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../../Shared/Models.h"
#include "../../Audio/PluginProcessor.h"

/**
 * Class to manage musical scales and note modifications
 */
class ScaleManager {
public:
    ScaleManager(PluginProcessor &p);

    ~ScaleManager() = default;

    // Apply scale and modifications to a note according to the settings
    int applyScaleAndModifications(int noteNumber);

    // Check if a note is in the specified scale
    bool isNoteInScale(int note, const juce::Array<int> &scale);

    // Find the closest note in scale to the given note
    int findClosestNoteInScale(int note, const juce::Array<int> &scale, int root);

    // Get the scale array based on scale type
    juce::Array<int> getSelectedScale(Models::ScaleType scaleType);

    // Reset arpeggiator state
    void resetArpeggiator();

private:
    PluginProcessor &processor;

    std::unique_ptr<AppState::ParameterBinding<Models::MelodySettings>> paramBinding;
    Models::MelodySettings settings;

    // Arpeggiator state
    int currentArpStep = 0;
    bool arpDirectionUp = true;
}; 