//
// Created by Shaked Melman on 01/03/2025.
//

#pragma once

#include "BaseSection.h"
#include "../Components/DirectionSelector.h"
#include "../../Audio/Params.h"
#include <juce_audio_processors/juce_audio_processors.h>

class SampleSectionComponent : public BaseSectionComponent,
                               public juce::TableListBoxModel,
                               public juce::FileDragAndDropTarget
{
public:
    SampleSectionComponent(PluginEditor& editor, PluginProcessor& processor);
    ~SampleSectionComponent() override;

    // TableListBoxModel overrides
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    void deleteKeyPressed(int currentSelectedRow) override;

    // Component overrides
    void resized() override;
    void paint(juce::Graphics& g) override;

    // FileDragAndDropTarget overrides
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;

private:
    // UI Components
    std::unique_ptr<juce::TableListBox> sampleListBox;
    std::unique_ptr<juce::TextButton> addSampleButton;
    std::unique_ptr<juce::TextButton> removeSampleButton;
    std::unique_ptr<juce::Label> sampleNameLabel;

    // Selected state
    std::set<int> selectedSamples;
    int lastSelectedSample = -1;
    
    // Replace randomize controls with direction selector
    std::unique_ptr<DirectionSelector> sampleDirectionSelector;
    
    bool draggedOver = false;
};
