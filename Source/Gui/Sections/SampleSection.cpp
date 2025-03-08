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

    // Sample section layout
    int controlsY = 40;

    // Sample list area - takes up left side
    int sampleListWidth = static_cast<int>(area.getWidth() * 0.6f);
    int sampleListHeight = area.getHeight() - 70;
    juce::Rectangle<int> listArea(area.getX() + 10, controlsY, sampleListWidth, sampleListHeight);

    // Set both the list box and detail view to occupy the same space for seamless transition
    sampleList->setBounds(listArea);
    sampleDetailView->setBounds(listArea);

    // Right side controls - direction selector and group list
    int controlsX = area.getX() + sampleListWidth + 15;
    int controlsWidth = area.getWidth() - sampleListWidth - 15;

    sampleDirectionSelector->setBounds(
        10 + sampleListWidth / 2 - 40,
        getHeight() - 27,
        80,
        25);
        
    // Position the Clear All button in the bottom right
    clearAllButton->setBounds(
        listArea.getRight() - 80,
        listArea.getBottom() + 5,
        80,
        25);
        
    // Position the group list view below the direction selector
    groupListView->setBounds(
        controlsX,
        40,
        controlsWidth,
        sampleListHeight - 50);

    const int toggleWidth = 60;
    const int toggleHeight = 20;
    const int toggleX = area.getRight() - toggleWidth - 25;
    const int toggleY = 7;
    pitchFollowToggle->setBounds(toggleX, toggleY, toggleWidth, toggleHeight);
}

void SampleSectionComponent::initComponents(PluginProcessor& processorRef)
{

    juce::Rectangle<int> area = getLocalBounds();
    // Create the sample list component
    sampleList = std::make_unique<SampleList>(processorRef);
    sampleList->onSampleDetailRequested = [this](int sampleIndex) {
        showDetailViewForSample(sampleIndex);
    };
    addAndMakeVisible(sampleList.get());

    // Create the detail view component but don't make it visible yet
    sampleDetailView = std::make_unique<SampleDetailComponent>(processorRef.getSampleManager());
    sampleDetailView->onBackButtonClicked = [this]() { showListView(); };
    addChildComponent(sampleDetailView.get()); // Add as child but not visible

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
    
    // Create the group list view
    groupListView = std::make_unique<GroupListView>(processorRef);
    addAndMakeVisible(groupListView.get());

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
            param->setValueNotifyingHost(followPitch ? 1.0f : 0.0f);
            param->endChangeGesture();
        }
    };
    addAndMakeVisible(pitchFollowToggle.get());

    // Create label
    pitchFollowLabel = std::make_unique<juce::Label>();
    pitchFollowLabel->setText("ORIG | PITCH", juce::dontSendNotification);
    pitchFollowLabel->setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    pitchFollowLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(pitchFollowLabel.get());
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
    // Hide detail view, show list view
    sampleDetailView->setVisible(false);
    sampleList->setVisible(true);

    // Update state
    showingDetailView = false;

    // Force a full layout refresh
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

        // Show detail view, hide list view
        sampleList->setVisible(false);
        sampleDetailView->setVisible(true);

        // Update state
        showingDetailView = true;

        repaint();
    }
}