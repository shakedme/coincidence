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
    sampleListBox->setHeaderHeight(20); // Reduced from 22
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
        std::make_unique<DirectionSelector>("PLAYBACK MODE", juce::Colour(0xffbf52d9));

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
}

SampleSectionComponent::~SampleSectionComponent()
{
    clearAttachments();
}

void SampleSectionComponent::resized()
{
    auto area = getLocalBounds();

    // Sample section layout
    int controlsY = 40; // Reduced from 40

    // Sample list takes up left side
    int sampleListWidth = area.getWidth() * 0.6f;
    sampleListBox->setBounds(
        area.getX() + 10, controlsY, sampleListWidth, area.getHeight() - 70);

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
        60); // Height with enough room for label
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

int SampleSectionComponent::getNumRows()
{
    return processor.getSampleManager().getNumSamples();
}

void SampleSectionComponent::paintRowBackground(
    juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colour(0x80bf52d9)); // Purple highlight
    else if (rowNumber % 2)
        g.fillAll(juce::Colour(0xff3a3a3a)); // Darker
    else
        g.fillAll(juce::Colour(0xff444444)); // Lighter
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
            g.drawText(processor.getSampleManager().getSampleName(rowNumber),
                       2,
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
        processor.getSampleManager().removeSamples(selectedRows[0], selectedRows.size() - 1);
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