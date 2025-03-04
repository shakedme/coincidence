//
// Created by Shaked Melman on 01/03/2025.
//

#include "SampleSection.h"

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

    int controlsY = 40; // Reduced from 40

    int sampleListWidth = area.getWidth() * 0.6f;
    sampleListBox->setBounds(
        area.getX() + 10, controlsY, sampleListWidth, area.getHeight() - 70);

    int controlsX = area.getX() + sampleListWidth + 15; // Reduced from 20
    int controlsWidth = area.getWidth() - sampleListWidth - 25;

    sampleDirectionSelector->setBounds(
        controlsX + (controlsWidth - 80) / 2, controlsY + 60, 80, 25);
}

void SampleSectionComponent::paint(juce::Graphics& g)
{
    // Call the base class paint method first to ensure we get the section header
    BaseSectionComponent::paint(g);

    // Check if we have any samples loaded
    bool noSamplesLoaded = processor.getSampleManager().getNumSamples() == 0;

    // Get the content area (excluding the header area)
    auto contentArea = getLocalBounds().withTrimmedTop(40);

    // If no samples are loaded, show a message and disable controls
    if (noSamplesLoaded)
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
        sampleListBox->repaint();
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
            int currentPlayingSample =
                processor.getNoteGenerator().getCurrentActiveSampleIdx();

            // Check if this row is the currently playing sample
            bool isPlayingSample = (rowNumber == currentPlayingSample);

            // Add some extra padding for playing samples to account for the indicator
            int leftPadding = isPlayingSample ? 8 : 2;

            // Make the text brighter for the playing sample
            if (isPlayingSample)
                g.setColour(juce::Colours::white);

            g.drawText(processor.getSampleManager().getSampleName(rowNumber),
                       leftPadding,
                       0,
                       width - 4,
                       height,
                       juce::Justification::centredLeft);
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
        processor.getSampleManager().removeSamples(selectedRows[0],
                                                   selectedRows.size() - 1);
        sampleListBox->updateContent();
    }
}

bool SampleSectionComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
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