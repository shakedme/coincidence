#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../Audio/MidiGeneratorProcessor.h"
#include "MidiGeneratorLookAndFeel.h"
#include "Sections/GrooveSection.h"
#include "Sections/PitchSection.h"
#include "Sections/GlitchSection.h"
#include "Sections/SampleSection.h"


/**
 * MidiGeneratorEditor - Main editor class for the MIDI generator plugin
 */
class MidiGeneratorEditor
    : public juce::AudioProcessorEditor
    , private juce::Timer
    , public juce::FileDragAndDropTarget
{
public:
    MidiGeneratorEditor(MidiGeneratorProcessor&);
    ~MidiGeneratorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

    // Method to update keyboard state with played notes
    void updateKeyboardState(bool isNoteOn, int noteNumber, int velocity);
    
    // Method to update active sample display
    void updateActiveSample(int index);

    // FileDragAndDropTarget methods
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;

private:
    // Reference to the processor
    MidiGeneratorProcessor& processor;

    // Custom look and feel
    MidiGeneratorLookAndFeel customLookAndFeel;
    
    // UI Sections
    std::unique_ptr<GrooveSectionComponent> grooveSection;
    std::unique_ptr<PitchSectionComponent> pitchSection;
    std::unique_ptr<GlitchSectionComponent> glitchSection;
    std::unique_ptr<SampleSectionComponent> sampleSection;
    
    // Keyboard component and state
    std::unique_ptr<juce::MidiKeyboardState> keyboardState;
    std::unique_ptr<juce::MidiKeyboardComponent> keyboardComponent;
    bool keyboardNeedsUpdate = false;
    
    // File drag and drop state
    bool isCurrentlyOver = false;
    
    // Helper methods
    void setupKeyboard();
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiGeneratorEditor)
};
