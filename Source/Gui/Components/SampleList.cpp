#include "SampleList.h"

SampleList::SampleList(PluginProcessor& p)
    : processor(p)
{
    // Create sample list table
    sampleListBox = std::make_unique<juce::TableListBox>("Sample List", this);
    sampleListBox->setHeaderHeight(0); // No header
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

    sampleListBox->getHeader().addColumn("", 1, getWidth() - 8);
    addAndMakeVisible(sampleListBox.get());
}

void SampleList::resized()
{
    // Sample list takes up the entire component
    sampleListBox->setBounds(getLocalBounds());
    
    // Update the column width to match the component width
    sampleListBox->getHeader().setColumnWidth(1, getWidth() - 8);
}

int SampleList::getNumRows()
{
    return processor.getSampleManager().getNumSamples();
}

void SampleList::paintRowBackground(
    juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    // Check if this row is the currently playing sample
    bool isPlayingSample = (rowNumber == activeSampleIndex);

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
    }
}

void SampleList::cellClicked(int rowNumber, int columnId, const juce::MouseEvent& e)
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
        if (onSampleDetailRequested)
            onSampleDetailRequested(rowNumber);
        return;
    }

    // Check if clicked on delete icon
    if (e.x >= deleteIconX && e.x < deleteIconX + ICON_SIZE)
    {
        processor.getSampleManager().removeSamples(rowNumber, rowNumber);
        sampleListBox->updateContent();
        return;
    }

    juce::TableListBoxModel::cellClicked(rowNumber, columnId, e);
}

void SampleList::paintCell(juce::Graphics& g,
                           int rowNumber,
                           int columnId,
                           int width,
                           int height,
                           bool /*rowIsSelected*/)
{
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(14.0f)));

    if (rowNumber < processor.getSampleManager().getNumSamples())
    {
        if (columnId == 1) // Sample name
        {
            // Reserve space for the two icons on the right
            int textWidth = width - (ICON_SIZE + ICON_PADDING) * 2 - 8;

            g.drawText(processor.getSampleManager().getSampleName(rowNumber),
                       0,
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

void SampleList::deleteKeyPressed(int/*rowNumber*/)
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

void SampleList::setActiveSampleIndex(int index)
{
    if (activeSampleIndex != index)
    {
        activeSampleIndex = index;
        sampleListBox->repaint();
    }
}

void SampleList::updateContent()
{
    sampleListBox->updateContent();
}