#pragma once

#include "BaseSection.h"
#include "../Components/DirectionSelector.h"
#include "../Components/SampleList.h"
#include "../Components/SampleDetail.h"
#include "../Components/GroupListView.h"
#include "../../Audio/Sampler/OnsetDetector.h"
#include "../../Shared/Config.h"
#include "../Components/Toggle.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

// Custom tabbed component that allows us to handle tab changes
class SampleSectionTabs : public juce::TabbedComponent
{
public:
    explicit SampleSectionTabs(juce::TabbedButtonBar::Orientation orientation)
        : juce::TabbedComponent(orientation)
    {
        // Make sure we're not intercepting mouse events that should go to our content components
        setInterceptsMouseClicks(false, true);
        
        // Make container transparent
        setColour(juce::TabbedComponent::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::TabbedComponent::outlineColourId, juce::Colours::transparentBlack);
    }
    
    std::function<void(int)> onTabChanged;
    
    void currentTabChanged(int newCurrentTabIndex, const juce::String& /*newCurrentTabName*/) override
    {
        if (onTabChanged)
            onTabChanged(newCurrentTabIndex);
    }
};

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
    std::unique_ptr<juce::TextButton> clearAllButton;
    std::unique_ptr<juce::TextButton> normalizeButton;
    std::unique_ptr<Toggle> pitchFollowToggle;
    std::unique_ptr<juce::Label> pitchFollowLabel;

    // Tab components
    std::unique_ptr<SampleSectionTabs> tabs;
    enum TabIDs { SamplesTab = 0, GroupsTab = 1 };
    
    // Direction selector
    std::unique_ptr<DirectionSelector> sampleDirectionSelector;
    
    // Group list view
    std::unique_ptr<GroupListView> groupListView;

    // View state
    bool showingDetailView = false;
    int currentTabIndex = SamplesTab;

    // Track the currently active sample for highlighting
    int lastActiveSampleIndex = -1;

    bool draggedOver = false;

    void initComponents(PluginProcessor& processor);

    void showDetailViewForSample(int sampleIndex);
    void showListView();
    
    // Handle tab changes
    void handleTabChange(int newTabIndex);
    
    // Updates the visibility of components based on the current tab
    void updateTabVisibility();
};