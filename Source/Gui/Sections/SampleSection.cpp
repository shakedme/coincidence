//
// Created by Shaked Melman on 01/03/2025.
//

#include "SampleSection.h"
#include "../Components/SampleDetail.h"

SampleSectionComponent::SampleSectionComponent(PluginEditor& editor,
                                               PluginProcessor& processor)
    : BaseSectionComponent(editor, processor, "SAMPLE", juce::Colour(0xffbf52d9))
{
    // Create sample list table
    sampleListBox = std::make_unique<juce::TableListBox>("Sample List", this);
    sampleListBox->setHeaderHeight(0); // Reduced from 22
    sampleListBox->setMultipleSelectionEnabled(true); // Enable multiple selections

    // Make the list box transparent
    sampleListBox->setColour(juce::ListBox::backgroundColourId,
                             juce::Colours::transparentBlack);
    sampleListBox->setColour(juce::ListBox::outlineColourId,
                             juce::Colours::transparentBlack);
    sampleListBox->setColour(juce::TableListBox::backgroundColourId,
                             juce::Colours::transparentBlack);

    // Make header background transparent
    sampleListBox->getHeader().setColour(juce::TableHeaderComponent::backgroundColourId,
                                         juce::Colours::transparentBlack);
    sampleListBox->getHeader().setColour(juce::TableHeaderComponent::outlineColourId,
                                         juce::Colour(0xff4a4a4a));
    sampleListBox->getHeader().setColour(juce::TableHeaderComponent::textColourId,
                                         juce::Colours::white);

    // Add column with a better name that won't overflow
    sampleListBox->getHeader().addColumn("", 1, 200);
    addAndMakeVisible(sampleListBox.get());

    // Create the detail view component but don't make it visible yet
    sampleDetailView = std::make_unique<SampleDetailComponent>(processor.getSampleManager());
    sampleDetailView->onBackButtonClicked = [this]() { showListView(); };
    addChildComponent(sampleDetailView.get()); // Add as child but not visible

    removeSampleButton = std::make_unique<juce::TextButton>("Remove");
    addAndMakeVisible(removeSampleButton.get());

    // Sample direction selector (replacing randomization controls)
    sampleDirectionSelector =
        std::make_unique<DirectionSelector>(juce::Colour(0xffbf52d9));

    // Set initial value from parameter
    auto* sampleDirectionParam = dynamic_cast<juce::AudioParameterChoice*>(
        processor.parameters.getParameter("sample_direction"));

    if (sampleDirectionParam)
        sampleDirectionSelector->setDirection(
            static_cast<Params::DirectionType>(sampleDirectionParam->getIndex()));

    sampleDirectionSelector->onDirectionChanged =
        [this, &processor](Params::DirectionType direction)
    {
        auto* param = processor.parameters.getParameter("sample_direction");
        if (param)
        {
            param->beginChangeGesture();
        }
        param->setValueNotifyingHost(param->convertTo0to1(static_cast<int>(direction)));
        param->endChangeGesture();
    };
    addAndMakeVisible(sampleDirectionSelector.get());

    sampleNameLabel =
        std::unique_ptr<juce::Label>(createLabel("", juce::Justification::centred));
    sampleNameLabel->setFont(juce::Font(11.0f));
    addAndMakeVisible(sampleNameLabel.get());

    // Start timer to update the active sample highlight
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
    int controlsY = 40; // Reduced from 40

    // Sample list area - takes up left side
    int sampleListWidth = area.getWidth() * 0.6f;
    int sampleListHeight = area.getHeight() - 70;
    juce::Rectangle<int> listArea(area.getX() + 10, controlsY, sampleListWidth, sampleListHeight);

    // Set both the list box and detail view to occupy the same space for seamless transition
    sampleListBox->setBounds(listArea);
    sampleDetailView->setBounds(listArea);

    // Sample name display
    int sampleNameY = controlsY + area.getHeight() - 95;
    sampleNameLabel->setBounds(area.getX(), sampleNameY, sampleListWidth, 25);

    // Right side controls - now just the direction selector
    int controlsX = area.getX() + sampleListWidth + 15; // Reduced from 20
    int controlsWidth = area.getWidth() - sampleListWidth - 25;

    // Position the sample direction selector in the center of right panel
    sampleDirectionSelector->setBounds(
        controlsX + (controlsWidth - 80) / 2, // Center it horizontally
        controlsY + 60, // Place it in the middle vertically
        80, // Width
        25); // Height with enough room for label
}

void SampleSectionComponent::paint(juce::Graphics& g)
{
    // Call the base class paint method first to ensure we get the section header
    BaseSectionComponent::paint(g);

    // Check if we have any samples loaded
    bool noSamplesLoaded = processor.getSampleManager().getNumSamples() == 0;

    // Get the content area (excluding the header area)
    auto contentArea = getLocalBounds().withTrimmedTop(40);

    // If no samples are loaded and not showing detail view, show a message and disable controls
    if (noSamplesLoaded && !showingDetailView)
    {
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(juce::Font(14.0f));
        g.drawText("Drag & Drop Samples Here", contentArea, juce::Justification::centred);

        // Show drop zone
        if (draggedOver)
        {
            g.setColour(juce::Colours::white.withAlpha(0.2f));
            g.drawRect(contentArea.reduced(10), 2);
        }

        // Hide the sample direction selector when no samples are loaded
        sampleDirectionSelector->setVisible(false);
    }
    else
    {
        // Show the sample direction selector when samples are loaded
        sampleDirectionSelector->setVisible(true);
    }
}

void SampleSectionComponent::timerCallback()
{
    // Get the currently active sample index from the processor
    int currentActiveSample = processor.getNoteGenerator().getCurrentActiveSampleIdx();

    // If the active sample has changed, update the table display
    if (currentActiveSample != lastActiveSampleIndex)
    {
        lastActiveSampleIndex = currentActiveSample;

        if (!showingDetailView)
        {
            sampleListBox->repaint();
        }

        // Update the sample name label if a valid sample is playing and not in detail view
        if (currentActiveSample >= 0 && currentActiveSample < processor.getSampleManager().getNumSamples() && !showingDetailView)
        {
            sampleNameLabel->setText("Playing: " + processor.getSampleManager().getSampleName(currentActiveSample),
                                     juce::dontSendNotification);
        }
        else if (!showingDetailView)
        {
            sampleNameLabel->setText("", juce::dontSendNotification);
        }
    }
}

int SampleSectionComponent::getNumRows()
{
    return processor.getSampleManager().getNumSamples();
}

void SampleSectionComponent::paintRowBackground(
    juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    // Get current playing sample index from the processor
    int currentPlayingSample = processor.getNoteGenerator().getCurrentActiveSampleIdx();

    // Check if this row is the currently playing sample
    bool isPlayingSample = (rowNumber == currentPlayingSample);

    if (rowIsSelected)
        g.fillAll(juce::Colour(0x80bf52d9)); // Purple highlight
    else if (isPlayingSample)
        g.fillAll(juce::Colour(0xff7030a0)); // Brighter purple for playing sample
    else if (rowNumber % 2)
        g.fillAll(juce::Colour(0xff3a3a3a)); // Darker
    else
        g.fillAll(juce::Colour(0xff444444)); // Lighter

    // Add a glow effect to the playing sample
    if (isPlayingSample)
    {
        // Draw a subtle border
        g.setColour(juce::Colour(0xffbf52d9).withAlpha(0.6f));
        g.drawRect(0, 0, width, height, 1);

        // Add a small play indicator at left
        int indicatorWidth = 4;
        g.setColour(juce::Colour(0xffbf52d9));
        g.fillRect(0, 0, indicatorWidth, height);
    }
}

void SampleSectionComponent::cellClicked(int rowNumber, int columnId, const juce::MouseEvent& e)
{
    // Only process clicks in the sample name column
    if (columnId != 1)
        return;

    // Calculate icon positions
    int width = sampleListBox->getHeader().getColumnWidth(columnId);
    int pencilIconX = width - (ICON_SIZE + ICON_PADDING) * 2;
    int deleteIconX = width - (ICON_SIZE + ICON_PADDING);

    // Check if clicked on pencil (edit) icon
    if (e.x >= pencilIconX && e.x < pencilIconX + ICON_SIZE)
    {
        showDetailViewForSample(rowNumber);
        return;
    }

    if (e.x >= deleteIconX && e.x < deleteIconX + ICON_SIZE)
    {
        sampleDetailView->clearSampleData();
        processor.getSampleManager().removeSamples(rowNumber, rowNumber);
        sampleListBox->updateContent();
        return;
    }

    juce::TableListBoxModel::cellClicked(rowNumber, columnId, e);
}

void SampleSectionComponent::paintCell(juce::Graphics& g,
                                       int rowNumber,
                                       int columnId,
                                       int width,
                                       int height,
                                       bool rowIsSelected)
{
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);

    if (rowNumber < processor.getSampleManager().getNumSamples())
    {
        if (columnId == 1) // Sample name
        {
            // Get current playing sample index from the processor
            int currentPlayingSample = processor.getNoteGenerator().getCurrentActiveSampleIdx();

            // Check if this row is the currently playing sample
            bool isPlayingSample = (rowNumber == currentPlayingSample);

            // Add some extra padding for playing samples to account for the indicator
            int leftPadding = isPlayingSample ? 8 : 2;

            // Make the text brighter for the playing sample
            if (isPlayingSample)
                g.setColour(juce::Colours::white);

            // Reserve space for the two icons on the right
            int textWidth = width - (ICON_SIZE + ICON_PADDING) * 2 - 8;

            g.drawText(processor.getSampleManager().getSampleName(rowNumber),
                       leftPadding,
                       0,
                       textWidth,
                       height,
                       juce::Justification::centredLeft);

            // Draw the icons on the right side

            // Calculate icon positions
            int pencilIconX = width - (ICON_SIZE + ICON_PADDING) * 2;
            int deleteIconX = width - (ICON_SIZE + ICON_PADDING);
            int iconY = (height - ICON_SIZE) / 2;

            // Draw pencil (edit) icon
            g.setColour(juce::Colours::lightgrey);
            juce::Path pencilPath;
            float penSize = ICON_SIZE * 0.8f;
            float penX = pencilIconX + (ICON_SIZE - penSize) / 2;
            float penY = iconY + (ICON_SIZE - penSize) / 2;

            // Pencil body
            pencilPath.startNewSubPath(penX, penY + penSize);
            pencilPath.lineTo(penX + penSize * 0.7f, penY + penSize);
            pencilPath.lineTo(penX + penSize, penY + penSize * 0.7f);
            pencilPath.lineTo(penX + penSize * 0.3f, penY);
            pencilPath.lineTo(penX, penY + penSize * 0.3f);
            pencilPath.closeSubPath();

            // Pencil tip
            pencilPath.addTriangle(penX + penSize * 0.3f, penY,
                                   penX + penSize * 0.15f, penY - penSize * 0.15f,
                                   penX + penSize * 0.45f, penY + penSize * 0.15f);

            g.fillPath(pencilPath);

            // Draw X (delete) icon
            g.setColour(juce::Colours::lightgrey);
            float crossSize = ICON_SIZE * 0.6f;
            float crossX = deleteIconX + (ICON_SIZE - crossSize) / 2;
            float crossY = iconY + (ICON_SIZE - crossSize) / 2;

            g.drawLine(crossX, crossY, crossX + crossSize, crossY + crossSize, 2.0f);
            g.drawLine(crossX, crossY + crossSize, crossX + crossSize, crossY, 2.0f);
        }
    }
}

void SampleSectionComponent::deleteKeyPressed(int currentSelectedRow)
{
    // Get all selected rows
    auto selectedRows = sampleListBox->getSelectedRows();

    // If there are multiple selections, remove them all
    if (selectedRows.size() > 0)
    {
        processor.getSampleManager().removeSamples(selectedRows[0], selectedRows.size() - 1);
        sampleListBox->updateContent();
    }
}

bool SampleSectionComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    // Only accept file drags when showing the list view
    if (showingDetailView)
        return false;

    // Check if any of the files are audio files - expanded file extension list
    for (int i = 0; i < files.size(); ++i)
    {
        juce::File f(files[i]);
        if (f.hasFileExtension("wav;aif;aiff;mp3;flac;ogg;m4a;wma"))
            return true;
    }
    return false;
}

void SampleSectionComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    // First, reset the drag state
    draggedOver = false;

    // Process the dropped files
    bool needsUpdate = false;

    for (const auto& file: files)
    {
        juce::File f(file);
        if (f.existsAsFile() && f.hasFileExtension("wav;aif;aiff;mp3"))
        {
            DBG("Loading dropped sample: " + f.getFullPathName());
            processor.getSampleManager().addSample(f);
            needsUpdate = true;
        }
    }

    if (needsUpdate)
    {
        sampleListBox->updateContent();
        repaint();
    }

    // Update sample name label
    if (processor.getSampleManager().getNumSamples() == 0)
    {
        sampleNameLabel->setText("No samples loaded", juce::dontSendNotification);
    }
    else
    {
        sampleNameLabel->setText("", juce::dontSendNotification);
    }
}

void SampleSectionComponent::fileDragEnter(const juce::StringArray& files, int x, int y)
{
    // Check if any of the files are valid audio files
    bool hasValidFiles = false;
    for (const auto& file: files)
    {
        juce::File f(file);
        if (f.hasFileExtension("wav;aif;aiff;mp3;ogg;flac"))
        {
            hasValidFiles = true;
            break;
        }
    }

    if (hasValidFiles)
    {
        draggedOver = true;
        repaint();
    }
}

void SampleSectionComponent::fileDragExit(const juce::StringArray& files)
{
    draggedOver = false;
    repaint();
}

void SampleSectionComponent::showListView()
{
    // Hide detail view, show list view
    sampleDetailView->setVisible(false);
    sampleListBox->setVisible(true);

    // Update state
    showingDetailView = false;
    detailViewSampleIndex = -1;

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
        sampleListBox->setVisible(false);
        sampleDetailView->setVisible(true);

        // Update state
        showingDetailView = true;
        detailViewSampleIndex = sampleIndex;

        repaint();
    }
}