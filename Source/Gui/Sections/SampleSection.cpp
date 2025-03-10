#include "SampleSection.h"

SampleSectionComponent::SampleSectionComponent(PluginEditor& editorRef,
                                               PluginProcessor& processorRef)
    : BaseSectionComponent(editorRef, processorRef, "SAMPLE", juce::Colour(0xffbf52d9))
{
    initComponents(processorRef);
    startTimerHz(30);
}

SampleSectionComponent::~SampleSectionComponent()
{
    stopTimer();
    clearAttachments();
}

void SampleSectionComponent::resized()
{
    auto area = getLocalBounds();
    
    // Position the tabs in the header area - moved further away from left edge and reduced height
    tabs->setBounds(30, 5, 200, 25);

    // Main content area starts after the header
    int contentY = 40;
    int contentHeight = area.getHeight() - 70; // Reduced by header and padding
    juce::Rectangle<int> contentArea(area.getX() + 10, contentY, area.getWidth() - 20, contentHeight);
    
    // Set both the list box and detail view to occupy the same space for seamless transition
    sampleList->setBounds(contentArea);
    sampleDetailView->setBounds(contentArea);
    
    // Position the group list to occupy full content area
    groupListView->setBounds(contentArea);

    // Position the Direction selector at the bottom
    sampleDirectionSelector->setBounds(
        10 + contentArea.getWidth() / 2 - 40,
        getHeight() - 27,
        80,
        25);
        
    // Position the Clear All button in the bottom right
    clearAllButton->setBounds(
        contentArea.getRight() - 80,
        contentArea.getBottom() + 5,
        80,
        25);
    
    // Position the Normalize button to the left of Clear All button
    normalizeButton->setBounds(
        contentArea.getRight() - 170, // 80 (Clear All width) + 10 (padding) + 80 (Normalize width)
        contentArea.getBottom() + 5,
        80,
        25);

    const int toggleWidth = 60;
    const int toggleHeight = 20;
    const int toggleX = area.getRight() - toggleWidth - 25;
    const int toggleY = 7;
    pitchFollowToggle->setBounds(toggleX, toggleY, toggleWidth, toggleHeight);
    
    // Update component visibility based on current tab
    updateTabVisibility();
}

void SampleSectionComponent::initComponents(PluginProcessor& processorRef)
{
    // Create the group list view first (bottom layer)
    groupListView = std::make_unique<GroupListView>(processorRef);
    addChildComponent(groupListView.get()); // Add as child but not visible initially
    
    // Create the sample list component next
    sampleList = std::make_unique<SampleList>(processorRef);
    sampleList->onSampleDetailRequested = [this](int sampleIndex) {
        showDetailViewForSample(sampleIndex);
    };
    addAndMakeVisible(sampleList.get());

    // Create the detail view component but don't make it visible yet
    sampleDetailView = std::make_unique<SampleDetailComponent>(processorRef.getSampleManager());
    sampleDetailView->onBackButtonClicked = [this]() { showListView(); };
    addChildComponent(sampleDetailView.get()); // Add as child but not visible
    
    // Now create our custom tab component with tabs at the top (top layer for tab bar only)
    tabs = std::make_unique<SampleSectionTabs>(juce::TabbedButtonBar::TabsAtTop);
    tabs->addTab("Samples", juce::Colour(0xffbf52d9), nullptr, false);
    tabs->addTab("Groups", juce::Colour(0xffbf52d9), nullptr, false);
    tabs->onTabChanged = [this](int newTabIndex) {
        handleTabChange(newTabIndex);
    };
    tabs->setCurrentTabIndex(currentTabIndex);
    tabs->setOutline(0); // Remove outline
    tabs->setTabBarDepth(25); // Reduced from HEADER_HEIGHT (30) to 25
    addAndMakeVisible(tabs.get());

    removeSampleButton = std::make_unique<juce::TextButton>("Remove");
    addAndMakeVisible(removeSampleButton.get());

    // Create Clear All button
    clearAllButton = std::make_unique<juce::TextButton>("Clear All");
    clearAllButton->setColour(juce::TextButton::buttonColourId, juce::Colour(0xffbf52d9));
    clearAllButton->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    clearAllButton->onClick = [this]() {
        // If in detail view, switch back to list view first to avoid dangling references
        if (showingDetailView) {
            showListView();
        }
        
        // Make sure we clear any sample data in the detail view to avoid dangling references
        sampleDetailView->clearSampleData();
        
        // Reset active sample state
        lastActiveSampleIndex = -1;
        
        // Clear all samples using the SampleManager
        processor.getSampleManager().clearAllSamples();
        
        // Update the sample list display
        sampleList->updateContent();
        
        // Repaint to show empty state UI
        repaint();
    };
    addAndMakeVisible(clearAllButton.get());

    // Create Normalize button
    normalizeButton = std::make_unique<juce::TextButton>("Normalize");
    normalizeButton->setColour(juce::TextButton::buttonColourId, juce::Colour(0xffbf52d9));
    normalizeButton->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    normalizeButton->onClick = [this]() {
        // If no samples, do nothing
        if (processor.getSampleManager().getNumSamples() == 0) {
            return;
        }
        
        // Normalize all samples
        processor.getSampleManager().normalizeSamples();
        
        // If in detail view, update waveform display
        if (showingDetailView) {
            sampleDetailView->rebuildWaveform();
        }
    };
    addAndMakeVisible(normalizeButton.get());

    // Sample direction selector
    sampleDirectionSelector = std::make_unique<DirectionSelector>(juce::Colour(0xffbf52d9));

    // Set initial value from parameter
    auto* sampleDirectionParam = dynamic_cast<juce::AudioParameterChoice*>(
        processorRef.parameters.getParameter("sample_direction"));

    if (sampleDirectionParam)
        sampleDirectionSelector->setDirection(
            static_cast<Params::DirectionType>(sampleDirectionParam->getIndex()));

    sampleDirectionSelector->onDirectionChanged =
        [&processorRef](Params::DirectionType direction)
    {
        auto* param = processorRef.parameters.getParameter("sample_direction");
        if (param)
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<int>(direction)));
            param->endChangeGesture();
        }
    };
    addAndMakeVisible(sampleDirectionSelector.get());

    pitchFollowToggle = std::make_unique<Toggle>(juce::Colour(0xffbf52d9));
    pitchFollowToggle->setTooltip("Enable pitch following for sample playback");

    auto* pitchFollowParam = dynamic_cast<juce::AudioParameterBool*>(
        processorRef.parameters.getParameter("sample_pitch_follow"));

    if (pitchFollowParam)
        pitchFollowToggle->setValue(pitchFollowParam->get());

    pitchFollowToggle->onValueChanged = [&processorRef](bool followPitch) {
        auto* param = processorRef.parameters.getParameter("sample_pitch_follow");
        if (param) {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(followPitch));
            param->endChangeGesture();
        }
    };
    addAndMakeVisible(pitchFollowToggle.get());

    pitchFollowLabel = std::make_unique<juce::Label>();
    pitchFollowLabel->setText("Pitch Follow", juce::dontSendNotification);
    pitchFollowLabel->setFont(juce::Font(11.0f));
    pitchFollowLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    pitchFollowLabel->setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(pitchFollowLabel.get());
    
    // Set initial tab visibility
    updateTabVisibility();
    
    // Ensure proper z-ordering of components
    if (currentTabIndex == SamplesTab) {
        sampleList->toFront(false);
    } else {
        groupListView->toFront(false);
    }
    
    // Tab bar and common controls should always be on top
    tabs->toFront(false);
    sampleDirectionSelector->toFront(false);
    clearAllButton->toFront(false);
    normalizeButton->toFront(false);
    pitchFollowToggle->toFront(false);
    pitchFollowLabel->toFront(false);
}

// Handle tab changes
void SampleSectionComponent::handleTabChange(int newTabIndex)
{
    currentTabIndex = newTabIndex;
    updateTabVisibility();
    
    // Ensure proper z-ordering when switching tabs
    if (currentTabIndex == SamplesTab) {
        if (showingDetailView) {
            sampleDetailView->toFront(false);
        } else {
            sampleList->toFront(false);
        }
    } else {
        groupListView->toFront(false);
    }
    
    // Tab bar and common controls should always be on top
    tabs->toFront(false);
    sampleDirectionSelector->toFront(false);
    clearAllButton->toFront(false);
    normalizeButton->toFront(false);
    pitchFollowToggle->toFront(false);
    pitchFollowLabel->toFront(false);
}

// Update component visibility based on current tab
void SampleSectionComponent::updateTabVisibility()
{
    // First, hide all content components
    sampleList->setVisible(false);
    sampleDetailView->setVisible(false);
    groupListView->setVisible(false);
    
    // Then show only the components for the current tab
    if (currentTabIndex == SamplesTab) {
        // In Samples tab, show sample list (unless detail view is active)
        if (!showingDetailView) {
            sampleList->setVisible(true);
        } else {
            sampleDetailView->setVisible(true);
        }
    } else if (currentTabIndex == GroupsTab) {
        // In Groups tab, show only the group list view
        groupListView->setVisible(true);
    }

    // Always show the direction selector and other common controls
    sampleDirectionSelector->setVisible(true);
    clearAllButton->setVisible(true);
    normalizeButton->setVisible(true);
    
    // Make sure our tab container doesn't capture mouse events intended for child components
    tabs->setInterceptsMouseClicks(false, true);
    
    // Force a repaint to ensure clean display
    repaint();
}

void SampleSectionComponent::paint(juce::Graphics& g)
{
    // Call the base class paint method first to ensure we get the section header
    BaseSectionComponent::paint(g);

    // Check if we have any samples loaded
    bool noSamplesLoaded = processor.getSampleManager().getNumSamples() == 0;

    // Get the content area (excluding the header area)
    auto contentArea = getLocalBounds().withTrimmedTop(40);

    // If no samples are loaded and not showing detail view, handle drop zone
    if (noSamplesLoaded && !showingDetailView)
    {
        // Draw drop zone border when dragging
        if (draggedOver)
        {
            g.setColour(juce::Colours::white.withAlpha(0.2f));
            g.drawRect(contentArea.reduced(10), 2);
        }

        // Draw the drag & drop text centered in the content area
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(juce::Font(juce::FontOptions(14.0f)));
        g.drawText("Drag & Drop Samples Here", contentArea, juce::Justification::centred, true);

        // Hide the sample direction selector, group list, and clear all button when no samples are loaded
        sampleDirectionSelector->setVisible(false);
        groupListView->setVisible(false);
        clearAllButton->setVisible(false);
    }
    else
    {
        // Show the sample direction selector, group list, and clear all button when samples are loaded
        sampleDirectionSelector->setVisible(true);
        groupListView->setVisible(true);
        clearAllButton->setVisible(true);
    }
}

void SampleSectionComponent::timerCallback()
{
    // Get the currently active sample index from the processor
    int currentActiveSample = processor.getNoteGenerator().getCurrentActiveSampleIdx();

    // If the active sample has changed, update the display
    if (currentActiveSample != lastActiveSampleIndex)
    {
        lastActiveSampleIndex = currentActiveSample;

        if (!showingDetailView)
        {
            // Update the sample list's active index
            sampleList->setActiveSampleIndex(currentActiveSample);
        }
    }
}

bool SampleSectionComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    // Only accept file drags when showing the list view
    if (showingDetailView)
        return false;

    // Check if any of the files are audio files
    for (int i = 0; i < files.size(); ++i)
    {
        juce::File f(files[i]);
        if (f.hasFileExtension("wav;aif;aiff;mp3;flac;ogg;m4a;wma"))
            return true;
    }
    return false;
}

void SampleSectionComponent::filesDropped(const juce::StringArray& files, int, int)
{
    // Reset the drag state
    draggedOver = false;

    // Process the dropped files
    bool needsUpdate = false;

    for (const auto& file: files)
    {
        juce::File f(file);
        if (f.existsAsFile() && f.hasFileExtension("wav;aif;aiff;mp3"))
        {
            processor.getSampleManager().addSample(f);
            needsUpdate = true;
        }
    }

    if (needsUpdate)
    {
        sampleList->updateContent();
        repaint();
    }
}

void SampleSectionComponent::fileDragEnter(const juce::StringArray&, int, int)
{
    draggedOver = true;
    repaint();
}

void SampleSectionComponent::fileDragExit(const juce::StringArray&)
{
    draggedOver = false;
    repaint();
}

void SampleSectionComponent::showListView()
{
    // Hide detail view
    sampleDetailView->setVisible(false);
    
    // Determine which components should be visible based on current tab
    if (currentTabIndex == SamplesTab) {
        sampleList->setVisible(true);
        groupListView->setVisible(false);
    } else {
        sampleList->setVisible(false);
        groupListView->setVisible(true);
    }

    // Update state
    showingDetailView = false;

    // Force a layout refresh
    resized();
    repaint();
}

void SampleSectionComponent::showDetailViewForSample(int sampleIndex)
{
    if (sampleDetailView->getSampleIndex() != sampleIndex) {
        sampleDetailView->clearSampleData();
    }

    if (sampleIndex >= 0 && sampleIndex < processor.getSampleManager().getNumSamples())
    {
        // Set up detail view for this sample
        sampleDetailView->setSampleIndex(sampleIndex);

        // Hide all other content components
        sampleList->setVisible(false);
        groupListView->setVisible(false);
        
        // Show detail view
        sampleDetailView->setVisible(true);

        // Update state
        showingDetailView = true;
        
        // Force a repaint
        repaint();
    }
}