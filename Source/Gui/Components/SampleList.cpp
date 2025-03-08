#include "SampleList.h"
#include "SampleNameCell.h"

SampleList::SampleList(PluginProcessor& p)
    : processor(p)
{
    // Create sample list table
    sampleListBox = std::make_unique<juce::TableListBox>("Sample List", this);
    sampleListBox->setHeaderHeight(24); // Add a header
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
                                         juce::Colour(0xff333333));
    sampleListBox->getHeader().setColour(juce::TableHeaderComponent::outlineColourId,
                                         juce::Colour(0xff4a4a4a));
    sampleListBox->getHeader().setColour(juce::TableHeaderComponent::textColourId,
                                         juce::Colours::white);

    // Add columns
    sampleListBox->getHeader().addColumn(
        "Name", 1, getWidth() * 0.6f, 80, -1, juce::TableHeaderComponent::notResizable);
    sampleListBox->getHeader().addColumn("Probability",
                                         2,
                                         getWidth() * 0.38f,
                                         80,
                                         -1,
                                         juce::TableHeaderComponent::notResizable);

    addAndMakeVisible(sampleListBox.get());
}

void SampleList::resized()
{
    // Sample list takes up the entire component
    sampleListBox->setBounds(getLocalBounds());

    // Update the column widths
    sampleListBox->getHeader().setColumnWidth(1, getWidth() * 0.6f);
    sampleListBox->getHeader().setColumnWidth(2, getWidth() * 0.4f);
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

    // Check if this sample is part of a group
    int groupIndex = -1;
    if (rowNumber >= 0 && rowNumber < processor.getSampleManager().getNumSamples())
    {
        // Use sampleInfo->groupIndex - it's more reliable than getting it from the SamplerSound
        auto sound = processor.getSampleManager().getSampleSound(rowNumber);
        if (sound != nullptr)
            groupIndex = sound->getGroupIndex();
    }

    // Set the background color based on selection, playing status, and group
    if (rowIsSelected)
    {
        g.fillAll(juce::Colour(0x80bf52d9)); // Purple highlight
    }
    else if (isPlayingSample)
    {
        g.fillAll(juce::Colour(0xff7030a0)); // Brighter purple for playing sample
    }
    else if (groupIndex >= 0)
    {
        // Use the group's color with transparency for the background
        g.fillAll(getGroupColor(groupIndex).withAlpha(0.4f));
    }
    else if (rowNumber % 2)
    {
        g.fillAll(juce::Colour(0xff3a3a3a)); // Darker
    }
    else
    {
        g.fillAll(juce::Colour(0xff444444)); // Lighter
    }

    // Add a glow effect or border to highlight samples
    if (isPlayingSample)
    {
        // Draw a subtle border for the playing sample
        g.setColour(juce::Colour(0xffbf52d9).withAlpha(0.6f));
        g.drawRect(0, 0, width, height, 1);
    }
    else if (groupIndex >= 0)
    {
        // Draw a subtle border with the group color
        g.setColour(getGroupColor(groupIndex).withAlpha(0.8f));
        g.drawRect(0, 0, width, height, 1);
    }
}

void SampleList::cellClicked(int rowNumber, int columnId, const juce::MouseEvent& e)
{
    // Only process clicks in the sample name column
    if (columnId != 1)
        return;

    // If right mouse button, show context menu
    if (e.mods.isRightButtonDown())
    {
        // Select this row if it's not already selected
        if (!sampleListBox->isRowSelected(rowNumber))
            sampleListBox->selectRow(rowNumber, false, false);

        // Show the context menu
        juce::PopupMenu menu;
        menu.addItem(CommandIDs::GroupSelected, "Group Selected Samples");
        menu.addItem(CommandIDs::UngroupSelected, "Ungroup Selected Samples");

        if (processor.getSampleManager().getNumGroups() > 0)
        {
            menu.addSeparator();
            menu.addItem(CommandIDs::RemoveGroups, "Remove All Groups");
        }

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                           [this](int menuItemID)
                           {
                               if (menuItemID != 0)
                                   menuItemSelected(menuItemID, 0);
                           });
        return;
    }

    // Let the TableListBoxModel handle other clicks
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
        if (columnId == 1) // Sample name column
        {
            // Reserve space for the icons on the right
            int textWidth = width - (ICON_SIZE + ICON_PADDING) * 3 - 8;

            // Get the sample name
            juce::String sampleName =
                processor.getSampleManager().getSampleName(rowNumber);

            // If the sample is part of a group, add an indicator
            auto sound = processor.getSampleManager().getSampleSound(rowNumber);
            if (sound != nullptr)
            {
                int groupIdx = sound->getGroupIndex();
                if (groupIdx >= 0
                    && groupIdx < processor.getSampleManager().getNumGroups())
                {
                    if (const SampleManager::Group* group =
                            processor.getSampleManager().getGroup(groupIdx))
                    {
                        sampleName += " [" + juce::String(groupIdx + 1) + "]";
                    }
                }
            }

            g.drawText(
                sampleName, 4, 0, textWidth, height, juce::Justification::centredLeft);
        }
        else if (columnId == 2) // Probability column - we'll use the slider cell
        {
            // Get the current probability value (0.0 - 1.0)
            float probability =
                processor.getSampleManager().getSampleProbability(rowNumber);

            // Display a text representation in the cell
            g.drawText(juce::String(int(probability * 100)) + "%",
                       4,
                       0,
                       width - 15,
                       height,
                       juce::Justification::centredLeft);
        }
    }
}

void SampleList::deleteKeyPressed(int /*rowNumber*/)
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

void SampleList::handleSliderValueChanged(int rowNumber, double value)
{
    if (rowNumber >= 0 && rowNumber < processor.getSampleManager().getNumSamples())
    {
        // Update the sample's probability
        processor.getSampleManager().setSampleProbability(rowNumber,
                                                          static_cast<float>(value));

        // Force a repaint of the cell to update the text display
        sampleListBox->repaintRow(rowNumber);
    }
}

juce::Component*
    SampleList::refreshComponentForCell(int rowNumber,
                                        int columnId,
                                        bool /*isRowSelected*/,
                                        juce::Component* existingComponentToUpdate)
{
    // Only create components for valid samples
    if (rowNumber >= processor.getSampleManager().getNumSamples())
        return nullptr;

    // Name column - add the icons
    if (columnId == 1)
    {
        // Get the sample sound
        auto* sound = processor.getSampleManager().getSampleSound(rowNumber);

        // Create new component or reuse existing
        auto* cellComponent =
            dynamic_cast<SampleNameCellComponent*>(existingComponentToUpdate);
        if (cellComponent == nullptr)
        {
            cellComponent = new SampleNameCellComponent(this, rowNumber, sound);
        }
        else
        {
            // If we're reusing, it's better to recreate to ensure state is fresh
            delete cellComponent;
            cellComponent = new SampleNameCellComponent(this, rowNumber, sound);
        }

        return cellComponent;
    }
    else if (columnId == 2) // Probability column
    {
        auto* sliderCell =
            dynamic_cast<ProbabilitySliderCell*>(existingComponentToUpdate);

        if (sliderCell == nullptr)
        {
            sliderCell = new ProbabilitySliderCell(this, rowNumber);
        }

        // Set the slider value to the current probability
        sliderCell->setValue(processor.getSampleManager().getSampleProbability(rowNumber),
                             juce::dontSendNotification);

        return sliderCell;
    }

    return nullptr;
}

void SampleList::menuItemSelected(int menuItemID, int /*topLevelMenuIndex*/)
{
    auto selectedRows = sampleListBox->getSelectedRows();

    switch (menuItemID)
    {
        case CommandIDs::GroupSelected:
        {
            if (selectedRows.size() > 0
                && processor.getSampleManager().getNumGroups() < 4)
            {
                // Convert the selected rows to a juce::Array
                juce::Array<int> indicesToGroup;
                for (int row = 0; row < selectedRows.size(); ++row)
                    indicesToGroup.add(selectedRows[row]);

                // Create a new group with these samples
                processor.getSampleManager().createGroup(indicesToGroup);

                // Refresh the list
                updateContent();
            }
            break;
        }

        case CommandIDs::UngroupSelected:
        {
            if (selectedRows.size() > 0)
            {
                // For each selected sample, remove it from any group
                for (int i = 0; i < selectedRows.size(); ++i)
                {
                    int row = selectedRows[i];
                    if (row >= 0 && row < processor.getSampleManager().getNumSamples())
                    {
                        // Use the proper method to remove a sample from its group
                        processor.getSampleManager().removeSampleFromGroup(row);
                    }
                }

                // Refresh the list
                updateContent();
            }
            break;
        }

        case CommandIDs::RemoveGroups:
        {
            // Remove all groups
            for (int i = processor.getSampleManager().getNumGroups() - 1; i >= 0; --i)
                processor.getSampleManager().removeGroup(i);

            // Refresh the list
            updateContent();
            break;
        }
    }
}

void SampleList::toggleOnsetRandomization(int sampleIndex)
{
    if (sampleIndex >= 0 && sampleIndex < processor.getSampleManager().getNumSamples())
    {
        auto* sound = processor.getSampleManager().getSampleSound(sampleIndex);
        if (sound != nullptr)
        {
            // Check if the sample has any onset markers
            if (sound->getOnsetMarkers().empty())
            {
                // No onset markers available, show warning
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "No Onset Markers",
                    "This sample doesn't have any onset markers yet. Please edit the sample to detect or add onset markers.",
                    "OK",
                    this);
            }
            else
            {
                // Toggle onset randomization
                bool newState = !sound->isOnsetRandomizationEnabled();
                sound->setOnsetRandomizationEnabled(newState);

                // Use updateContent instead of just repainting the row
                // This ensures the SampleNameCellComponent gets recreated with the correct icon state
                updateContent();
            }
        }
    }
}