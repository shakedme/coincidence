#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../Audio/PluginProcessor.h"
#include "LookAndFeel.h"
#include "Sections/GrooveSection.h"
#include "Sections/PitchSection.h"
#include "Sections/EffectsSection.h"
#include "Sections/SampleSection.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include "Sections/EnvelopeSection.h"
#include "Components/HeaderComponent.h"

class PluginEditor
    : public juce::AudioProcessorEditor
    , private juce::Timer
    , public juce::FileDragAndDropTarget
{
public:
    PluginEditor(PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

    void updateKeyboardState(bool isNoteOn, int noteNumber, int velocity);

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;

private:
    PluginProcessor& audioProcessor;
    LookAndFeel customLookAndFeel;
//    melatonin::Inspector inspector { *this };

    // Header component with tabs
    std::unique_ptr<HeaderComponent> header;

    // UI Sections
    std::unique_ptr<GrooveSectionComponent> grooveSection;
    std::unique_ptr<PitchSectionComponent> pitchSection;
    std::unique_ptr<EffectsSection> fxSection;
    std::unique_ptr<SampleSectionComponent> sampleSection;
    std::unique_ptr<EnvelopeSectionComponent> envelopeSection;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;

    // Keyboard component and state
    std::unique_ptr<juce::MidiKeyboardState> keyboardState;
    std::unique_ptr<juce::MidiKeyboardComponent> keyboardComponent;
    std::atomic<bool> keyboardNeedsUpdate = false;

    void setupKeyboard();
    void timerCallback() override;
    void switchTab(HeaderComponent::Tab tab);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
