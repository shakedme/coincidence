#include "SampleSection.h"

SampleSectionComponent::SampleSectionComponent(PluginEditor &editorRef,
                                               PluginProcessor &processorRef)
        : BaseSectionComponent(editorRef, processorRef, "SAMPLE", juce::Colour(0xffbf52d9)) {
    initComponents(processorRef);
    startTimerHz(30);
}

SampleSectionComponent::~SampleSectionComponent() {
    stopTimer();
    clearAttachments();
}

void SampleSectionComponent::resized() {
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

void SampleSectionComponent::initComponents(PluginProcessor &processorRef) {
    // Create the group list view first (bottom layer)
    groupListView = std::make_unique<GroupListView>(processorRef);
    addChildComponent(groupListView.get()); // Add as child but not visible initially

    // Create the sample list component next
    sampleList = std::make_unique<SampleList>(processorRef);
    sampleList->onSampleDetailRequested = [this](int sampleIndex) {
        // Only show detail view if we're in the Samples tab
        if (currentTabIndex == SamplesTab) {
            showDetailViewForSample(sampleIndex);
        }
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
    auto *sampleDirectionParam = dynamic_cast<juce::AudioParameterChoice *>(
            processorRef.getAPVTS().getParameter(Params::ID_SAMPLE_DIRECTION));

    if (sampleDirectionParam)
        sampleDirectionSelector->setDirection(
                static_cast<Models::DirectionType>(sampleDirectionParam->getIndex()));

    sampleDirectionSelector->onDirectionChanged =
            [&processorRef](Models::DirectionType direction) {
                auto *param = processorRef.getAPVTS().getParameter(Params::ID_SAMPLE_DIRECTION);
                if (param) {
                    param->beginChangeGesture();
                    param->setValueNotifyingHost(param->convertTo0to1(static_cast<int>(direction)));
                    param->endChangeGesture();
                }
            };
    addAndMakeVisible(sampleDirectionSelector.get());

    pitchFollowToggle = std::make_unique<Toggle>(juce::Colour(0xffbf52d9));
    pitchFollowToggle->setTooltip("Enable pitch following for sample playback");

    auto *pitchFollowParam = dynamic_cast<juce::AudioParameterBool *>(
            processorRef.getAPVTS().getParameter(Params::ID_SAMPLE_PITCH_FOLLOW));

    if (pitchFollowParam)
        pitchFollowToggle->setValue(pitchFollowParam->get());

    pitchFollowToggle->onValueChanged = [&processorRef](bool followPitch) {
        auto *param = processorRef.getAPVTS().getParameter(Params::ID_SAMPLE_PITCH_FOLLOW);
        if (param) {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(followPitch));
            param->endChangeGesture();
        }
    };
    addAndMakeVisible(pitchFollowToggle.get());

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
}

// Handle tab changes
void SampleSectionComponent::handleTabChange(int newTabIndex) {
    // Update tab index
    currentTabIndex = newTabIndex;

    // If switching to Groups tab while in detail view, exit detail view
    if (currentTabIndex == GroupsTab && showingDetailView) {
        showingDetailView = false;
    }

    // Update component visibility based on new tab
    updateTabVisibility();

    // Ensure proper z-ordering when switching tabs
    // We want to maintain the detail view's visibility state when changing tabs
    if (showingDetailView && currentTabIndex == SamplesTab) {
        // Detail view should be on top if it's showing and we're in Samples tab
        sampleDetailView->toFront(false);
    } else if (currentTabIndex == SamplesTab) {
        // Sample list on top for Samples tab
        sampleList->toFront(false);
    } else {
        // Group list on top for Groups tab
        groupListView->toFront(false);
    }

    // Tab bar and common controls should always be on top
    tabs->toFront(false);
    sampleDirectionSelector->toFront(false);
    clearAllButton->toFront(false);
    normalizeButton->toFront(false);
    pitchFollowToggle->toFront(false);
}

// Update component visibility based on current tab
void SampleSectionComponent::updateTabVisibility() {
    // First, hide all content components
    sampleList->setVisible(false);
    sampleDetailView->setVisible(false);
    groupListView->setVisible(false);

    // Then show only the appropriate component based on current tab and view state
    if (currentTabIndex == SamplesTab) {
        // In Samples tab, we can show either sample list or detail view
        if (showingDetailView) {
            sampleDetailView->setVisible(true);
        } else {
            sampleList->setVisible(true);
        }
    } else if (currentTabIndex == GroupsTab) {
        // In Groups tab, show only the group list view
        groupListView->setVisible(true);
        // Detail view is never visible in Groups tab
    }

    // Always show the common controls unless we have no samples
    bool noSamplesLoaded = processor.getSampleManager().getNumSamples() == 0;
    bool showControls = !noSamplesLoaded;

    sampleDirectionSelector->setVisible(showControls);
    clearAllButton->setVisible(showControls);
    normalizeButton->setVisible(showControls);

    // Make sure our tab container doesn't capture mouse events intended for child components
    tabs->setInterceptsMouseClicks(false, true);

    // Force a repaint to ensure clean display
    repaint();
}

void SampleSectionComponent::paint(juce::Graphics &g) {
    // Call the base class paint method first to ensure we get the section header
    BaseSectionComponent::paint(g);

    // Check if we have any samples loaded
    bool noSamplesLoaded = processor.getSampleManager().getNumSamples() == 0;

    // Get the content area (excluding the header area)
    auto contentArea = getLocalBounds().withTrimmedTop(40);

    // If no samples are loaded and not showing detail view, handle drop zone
    if (noSamplesLoaded && !showingDetailView) {
        // Draw drop zone border when dragging
        if (draggedOver) {
            g.setColour(juce::Colours::white.withAlpha(0.2f));
            g.drawRect(contentArea.reduced(10), 2);
        }

        // Only draw the drag & drop text if we're in the Samples tab
        if (currentTabIndex == SamplesTab) {
            // Draw the drag & drop text centered in the content area
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            g.setFont(juce::Font(juce::FontOptions(14.0f)));
            g.drawText("Drag & Drop Samples Here", contentArea, juce::Justification::centred, true);
        }
            // Show instructional text for the Groups tab
        else if (currentTabIndex == GroupsTab) {
            // Draw the groups info text centered in the content area
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            g.setFont(juce::Font(juce::FontOptions(14.0f)));
            g.drawText("Add samples first to create groups", contentArea, juce::Justification::centred, true);
        }

        // Hide buttons when no samples are loaded
        sampleDirectionSelector->setVisible(false);
        clearAllButton->setVisible(false);
        normalizeButton->setVisible(false);

        // Ensure no views are visible in empty state
        sampleList->setVisible(false);
        groupListView->setVisible(false);
        sampleDetailView->setVisible(false);
    } else {
        // Show controls when samples are loaded
        sampleDirectionSelector->setVisible(true);
        clearAllButton->setVisible(true);
        normalizeButton->setVisible(true);

        // The visibility of the views (sample list, detail view, group list)
        // is managed by updateTabVisibility() and should not be overridden here
    }
}

void SampleSectionComponent::timerCallback() {
    // Get the currently active sample index from the processor
    int currentActiveSample = processor.getNoteGenerator().getCurrentActiveSampleIdx();

    // If the active sample has changed, update the display
    if (currentActiveSample != lastActiveSampleIndex) {
        lastActiveSampleIndex = currentActiveSample;

        if (!showingDetailView) {
            // Update the sample list's active index
            sampleList->setActiveSampleIndex(currentActiveSample);
        }
    }
}

bool SampleSectionComponent::isInterestedInFileDrag(const juce::StringArray &files) {
    // Only accept file drags when showing the list view
    if (showingDetailView)
        return false;

    // Check if any of the files are audio files
    for (int i = 0; i < files.size(); ++i) {
        juce::File f(files[i]);
        if (f.hasFileExtension("wav;aif;aiff;mp3;flac;ogg;m4a;wma"))
            return true;
    }
    return false;
}

void SampleSectionComponent::filesDropped(const juce::StringArray &files, int, int) {
    // Reset the drag state
    draggedOver = false;

    // Process the dropped files
    bool needsUpdate = false;

    for (const auto &file: files) {
        juce::File f(file);
        if (f.existsAsFile() && f.hasFileExtension("wav;aif;aiff;mp3")) {
            processor.getSampleManager().addSample(f);
            needsUpdate = true;
        }
    }

    if (needsUpdate) {
        // Refresh the list content
        sampleList->updateContent();

        // Make sure we're showing the sample list (not the detail view)
        showingDetailView = false;

        // Force update of component visibility
        updateTabVisibility();

        // Ensure sample list is at the front and repaint
        sampleList->toFront(false);
        repaint();
    }
}

void SampleSectionComponent::fileDragEnter(const juce::StringArray &, int, int) {
    draggedOver = true;
    repaint();
}

void SampleSectionComponent::fileDragExit(const juce::StringArray &) {
    draggedOver = false;
    repaint();
}

void SampleSectionComponent::showListView() {
    // Update state
    showingDetailView = false;

    // Update component visibility based on current tab
    updateTabVisibility();

    // Force a layout refresh
    resized();
    sampleList->updateContent();
}

void SampleSectionComponent::showDetailViewForSample(int sampleIndex) {
    // Only allow showing detail view in Samples tab
    if (currentTabIndex != SamplesTab) {
        return;
    }

    if (sampleDetailView->getSampleIndex() != sampleIndex) {
        sampleDetailView->clearSampleData();
    }

    if (sampleIndex >= 0 && sampleIndex < processor.getSampleManager().getNumSamples()) {
        // Set up detail view for this sample
        sampleDetailView->setSampleIndex(sampleIndex);

        // Hide all other content components to ensure only detail view is visible
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