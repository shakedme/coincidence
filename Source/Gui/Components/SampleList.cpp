#include "SampleList.h"

// Custom component to contain a probability slider with fixed width


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
    sampleListBox->getHeader().addColumn("Name", 1, getWidth() * 0.5f, 80, -1, juce::TableHeaderComponent::notResizable);
    sampleListBox->getHeader().addColumn("Probability", 2, getWidth() * 0.3f, 80, -1, juce::TableHeaderComponent::notResizable);
    sampleListBox->getHeader().addColumn("Onsets", 3, getWidth() * 0.2f, 80, -1, juce::TableHeaderComponent::notResizable);
    
    addAndMakeVisible(sampleListBox.get());
}

void SampleList::resized()
{
    // Sample list takes up the entire component
    sampleListBox->setBounds(getLocalBounds());
    
    // Update the column widths
    sampleListBox->getHeader().setColumnWidth(1, getWidth() * 0.5f);
    sampleListBox->getHeader().setColumnWidth(2, getWidth() * 0.3f);
    sampleListBox->getHeader().setColumnWidth(3, getWidth() * 0.2f);
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
                          [this](int menuItemID) {
                              if (menuItemID != 0)
                                  menuItemSelected(menuItemID, 0);
                          });
        return;
    }

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

            // Get the sample name
            juce::String sampleName = processor.getSampleManager().getSampleName(rowNumber);
            
            // If the sample is part of a group, add an indicator
            auto sound = processor.getSampleManager().getSampleSound(rowNumber);
            if (sound != nullptr)
            {
                int groupIdx = sound->getGroupIndex();
                if (groupIdx >= 0 && groupIdx < processor.getSampleManager().getNumGroups())
                {
                    if (const SampleManager::Group* group = processor.getSampleManager().getGroup(groupIdx))
                    {
                        sampleName += " [G" + juce::String(groupIdx + 1) + "]";
                    }
                }
            }

            g.drawText(sampleName,
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
        else if (columnId == 2) // Probability column - we'll use a slider component
        {
            // Get the current probability value (0.0 - 1.0)
            float probability = processor.getSampleManager().getSampleProbability(rowNumber);
            
            // Display a text representation in the cell
            g.drawText(juce::String(int(probability * 100)) + "%",
                       4,
                       0,
                       width - 15,
                       height,
                       juce::Justification::centredLeft);
        }
        else if (columnId == 3) // Onsets column - we'll use a toggle button
        {
            // Get the current toggle state
            auto* sound = processor.getSampleManager().getSampleSound(rowNumber);
            bool isOnsetModeEnabled = sound ? sound->isOnsetModeEnabled() : false;
            
            // Draw a toggle button
            g.setColour(juce::Colours::lightgrey);
            float toggleSize = ICON_SIZE * 0.8f;
            float toggleX = width - (ICON_SIZE + ICON_PADDING) - (ICON_SIZE - toggleSize) / 2;
            float toggleY = height / 2;

            g.fillRect(toggleX, toggleY - toggleSize / 2, toggleSize, toggleSize);

            if (isOnsetModeEnabled)
            {
                g.setColour(juce::Colours::green);
                g.fillRect(toggleX + toggleSize / 4, toggleY - toggleSize / 4, toggleSize / 2, toggleSize / 2);
            }
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

void SampleList::handleSliderValueChanged(int rowNumber, double value)
{
    // Pass the new probability value to the sample manager
    if (rowNumber >= 0 && rowNumber < processor.getSampleManager().getNumSamples())
    {
        processor.getSampleManager().setSampleProbability(rowNumber, static_cast<float>(value));
        
        // Force a repaint of the cell to update the text display
        sampleListBox->repaintRow(rowNumber);
        
        // Debug output
        DBG("Probability changed for sample " + juce::String(rowNumber) + ": " + juce::String(value));
    }
}

void SampleList::handleOnsetToggleChanged(int rowNumber, bool toggleState)
{
    // Update the sample's onset mode
    if (rowNumber >= 0 && rowNumber < processor.getSampleManager().getNumSamples())
    {
        auto* sound = processor.getSampleManager().getSampleSound(rowNumber);
        if (sound)
        {
            sound->setOnsetModeEnabled(toggleState);
            
            // Debug output
            DBG("Onset mode changed for sample " + juce::String(rowNumber) + ": " + juce::String(toggleState ? "ON" : "OFF"));
        }
    }
}

juce::Component* SampleList::refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/, juce::Component* existingComponentToUpdate)
{
    // Only create a slider cell for the probability column
    if (columnId == 2 && rowNumber < processor.getSampleManager().getNumSamples())
    {
        auto* sliderCell = dynamic_cast<ProbabilitySliderCell*>(existingComponentToUpdate);
        
        if (sliderCell == nullptr)
        {
            sliderCell = new ProbabilitySliderCell(this, rowNumber);
        }
        else
        {
            // Update the row number in case it changed
            delete sliderCell;
            sliderCell = new ProbabilitySliderCell(this, rowNumber);
        }
        
        // Set the slider value to the current probability
        sliderCell->setValue(processor.getSampleManager().getSampleProbability(rowNumber), juce::dontSendNotification);
        
        return sliderCell;
    }
    else if (columnId == 3 && rowNumber < processor.getSampleManager().getNumSamples())
    {
        auto* toggleCell = dynamic_cast<OnsetToggleButtonCell*>(existingComponentToUpdate);
        
        if (toggleCell == nullptr)
        {
            toggleCell = new OnsetToggleButtonCell(this, rowNumber);
        }
        else
        {
            // Update the row number in case it changed
            delete toggleCell;
            toggleCell = new OnsetToggleButtonCell(this, rowNumber);
        }
        
        // Set the toggle state based on the sample's onset mode
        auto* sound = processor.getSampleManager().getSampleSound(rowNumber);
        bool isOnsetModeEnabled = sound ? sound->isOnsetModeEnabled() : false;
        toggleCell->setState(isOnsetModeEnabled, juce::dontSendNotification);
        
        return toggleCell;
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
            if (selectedRows.size() > 0 && 
                processor.getSampleManager().getNumGroups() < 4)
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