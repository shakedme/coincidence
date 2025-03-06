#pragma once

#include "BaseSection.h"
#include "../Components/DirectionSelector.h"
#include "../Components/SampleList.h"
#include "../Components/SampleDetail.h"
#include "../Components/GroupListView.h"
#include "../../Audio/Params.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

class SampleSectionComponent : public BaseSectionComponent,
                               public juce::FileDragAndDropTarget,
                               private juce::Timer
{
public:
    SampleSectionComponent(PluginEditor& editor, PluginProcessor& processor);
    ~SampleSectionComponent() override;

    // Component overrides
    void resized() override;
    void paint(juce::Graphics& g) override;

    // FileDragAndDropTarget overrides
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;

    // Timer callback for updating the active sample highlight
    void timerCallback() override;

private:
    // UI Components
    std::unique_ptr<SampleList> sampleList;
    std::unique_ptr<SampleDetailComponent> sampleDetailView;
    std::unique_ptr<juce::TextButton> removeSampleButton;

    // Direction selector
    std::unique_ptr<DirectionSelector> sampleDirectionSelector;
    
    // Group list view
    std::unique_ptr<GroupListView> groupListView;

    // View state
    bool showingDetailView = false;

    // Track the currently active sample for highlighting
    int lastActiveSampleIndex = -1;

    bool draggedOver = false;

    void initComponents(PluginProcessor& processor);

    void showDetailViewForSample(int sampleIndex);
    void showListView();

};