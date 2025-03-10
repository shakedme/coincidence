#include "SampleList.h"
#include "SampleRow.h"

SampleList::SampleList(PluginProcessor &p)
        : processor(p) {
    // Create sample list table
    sampleListBox = std::make_unique<juce::TableListBox>("Sample List", this);
    sampleListBox->setHeaderHeight(0); // Remove the header by setting height to 0
    sampleListBox->setMultipleSelectionEnabled(true); // Enable multiple selections

    // Disable automatic row selection toggling - we'll handle this manually
    sampleListBox->setClickingTogglesRowSelection(false);
    
    // Disable horizontal scrolling to prevent overflow
    sampleListBox->getHorizontalScrollBar().setVisible(false);

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

    // Add a single full width column
    sampleListBox->getHeader().addColumn(
            "Name", 1, getWidth(), 80, -1, juce::TableHeaderComponent::notResizable);

    addAndMakeVisible(sampleListBox.get());
}

void SampleList::paint(juce::Graphics &g) {
    // Fill the background with a dark grey
    g.fillAll(juce::Colour(0xff222222));
}

void SampleList::resized() {
    // Sample list takes up the entire component
    sampleListBox->setBounds(getLocalBounds());

    // Update the single column to exactly match the width of the viewport
    // This prevents horizontal scrollbar from appearing
    int viewportWidth = getWidth();
    if (sampleListBox->getViewport() != nullptr) {
        viewportWidth = sampleListBox->getViewport()->getViewWidth();
    }
    sampleListBox->getHeader().setColumnWidth(1, viewportWidth);

    // Ensure the entire row width matches the column width
    sampleListBox->setMinimumContentWidth(viewportWidth);
}

int SampleList::getNumRows() {
    return processor.getSampleManager().getNumSamples();
}

void SampleList::paintRowBackground(
        juce::Graphics &g, int rowNumber, int width, int height, bool rowIsSelected) {
    // Check if this row is the currently playing sample
    bool isPlayingSample = (rowNumber == activeSampleIndex);

    // Check if this sample is part of a group
    int groupIndex = -1;
    if (rowNumber >= 0 && rowNumber < processor.getSampleManager().getNumSamples()) {
        // Use sampleInfo->groupIndex - it's more reliable than getting it from the SamplerSound
        auto sound = processor.getSampleManager().getSampleSound(rowNumber);
        if (sound != nullptr)
            groupIndex = sound->getGroupIndex();
    }

    // Set the background color based on selection, playing status, and group
    if (rowIsSelected) {
        g.fillAll(juce::Colour(0x80bf52d9)); // Purple highlight
    } else if (isPlayingSample) {
        g.fillAll(juce::Colour(0xff7030a0)); // Brighter purple for playing sample
    } else if (groupIndex >= 0) {
        // Use the group's color with transparency for the background
        g.fillAll(getGroupColor(groupIndex).withAlpha(0.4f));
    } else if (rowNumber % 2) {
        g.fillAll(juce::Colour(0xff3a3a3a)); // Darker
    } else {
        g.fillAll(juce::Colour(0xff444444)); // Lighter
    }

    // Add a glow effect or border to highlight samples
    if (isPlayingSample) {
        // Draw a subtle border for the playing sample
        g.setColour(juce::Colour(0xffbf52d9).withAlpha(0.6f));
        g.drawRect(0, 0, width, height, 1);
    } else if (groupIndex >= 0) {
        // Draw a subtle border with the group color
        g.setColour(getGroupColor(groupIndex).withAlpha(0.8f));
        g.drawRect(0, 0, width, height, 1);
    }
}

void SampleList::cellClicked(int rowNumber, int columnId, const juce::MouseEvent &e) {
    // If right mouse button, show context menu
    if (e.mods.isRightButtonDown()) {
        // Only select this row if it's not already selected
        // and preserve the existing selection if this row is already in it
        auto selectedRows = sampleListBox->getSelectedRows();
        bool rowIsAlreadySelected = selectedRows.contains(rowNumber);

        if (!rowIsAlreadySelected) {
            // If the row isn't already selected, select it (and only it)
            sampleListBox->selectRow(rowNumber);
        }
        // If row is already selected, keep the current selection intact

        // Show the context menu
        juce::PopupMenu menu;
        menu.addItem(CommandIDs::GroupSelected, "Group Selected Samples");
        menu.addItem(CommandIDs::UngroupSelected, "Ungroup Selected Samples");

        if (processor.getSampleManager().getNumGroups() > 0) {
            menu.addSeparator();
            menu.addItem(CommandIDs::RemoveGroups, "Remove All Groups");
        }

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(sampleListBox.get()),
                           [this](int menuItemID) {
                               if (menuItemID != 0)
                                   menuItemSelected(menuItemID, 0);
                           });
        return;
    }

    // Standard selection handling for left-clicks
    if (columnId == 1) {
        if (e.mods.isShiftDown()) {
            // Shift-click: extend selection to this row
            auto selectedRows = sampleListBox->getSelectedRows();
            int lastSelectedRow = selectedRows.isEmpty() ? 0 : selectedRows[selectedRows.size() - 1];

            // Select range from last selected to current row
            sampleListBox->selectRangeOfRows(juce::jmin(lastSelectedRow, rowNumber),
                                             juce::jmax(lastSelectedRow, rowNumber));
        } else if (e.mods.isCommandDown() || e.mods.isCtrlDown()) {
            // Ctrl/Cmd click: toggle this row's selection
            if (sampleListBox->isRowSelected(rowNumber))
                sampleListBox->deselectRow(rowNumber);
            else
                sampleListBox->selectRow(rowNumber, true); // Preserve existing selection
        } else {
            // Normal click: select only this row
            sampleListBox->selectRow(rowNumber);
        }
    }
}

void SampleList::paintCell(juce::Graphics &g,
                           int rowNumber,
                           int columnId,
                           int width,
                           int height,
                           bool /*rowIsSelected*/) {
    // We're now using a custom component for the single column,
    // so we don't need to paint anything here.
    // The SampleNameCellComponent handles all painting.
}

void SampleList::deleteKeyPressed(int /*rowNumber*/) {
    // Get all selected rows
    auto selectedRows = sampleListBox->getSelectedRows();

    // If there are multiple selections, remove them all
    if (selectedRows.size() > 0) {
        processor.getSampleManager().removeSamples(selectedRows[0],
                                                   selectedRows.size() - 1);
        sampleListBox->updateContent();
    }
}

void SampleList::setActiveSampleIndex(int index) {
    if (activeSampleIndex != index) {
        activeSampleIndex = index;
        sampleListBox->repaint();
    }
}

void SampleList::updateContent() {
    sampleListBox->updateContent();
}

void SampleList::handleSliderValueChanged(int rowNumber, double value) {
    if (rowNumber >= 0 && rowNumber < processor.getSampleManager().getNumSamples()) {
        // Update the sample's probability
        processor.getSampleManager().setSampleProbability(rowNumber,
                                                          static_cast<float>(value));

        // Force a repaint of the cell to update the text display
        sampleListBox->repaintRow(rowNumber);
    }
}

juce::Component *
SampleList::refreshComponentForCell(int rowNumber,
                                    int columnId,
                                    bool /*isRowSelected*/,
                                    juce::Component *existingComponentToUpdate) {
    // Only create components for valid samples
    if (rowNumber >= processor.getSampleManager().getNumSamples())
        return nullptr;

    // Only handle column 1 (the name column) since it's now a single-column display
    if (columnId == 1) {
        // Get the sample sound
        auto *sound = processor.getSampleManager().getSampleSound(rowNumber);

        // Create new component or reuse existing
        auto *cellComponent =
                dynamic_cast<SampleRow *>(existingComponentToUpdate);
        if (cellComponent == nullptr) {
            cellComponent = new SampleRow(this, rowNumber, sound);
        } else {
            // If we're reusing, it's better to recreate to ensure state is fresh
            delete cellComponent;
            cellComponent = new SampleRow(this, rowNumber, sound);
        }

        return cellComponent;
    }

    return nullptr;
}

void SampleList::menuItemSelected(int menuItemID, int /*topLevelMenuIndex*/) {
    auto selectedRows = sampleListBox->getSelectedRows();

    switch (menuItemID) {
        case CommandIDs::GroupSelected: {
            if (selectedRows.size() > 0
                && processor.getSampleManager().getNumGroups() < 4) {
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

        case CommandIDs::UngroupSelected: {
            if (selectedRows.size() > 0) {
                // For each selected sample, remove it from any group
                for (int i = 0; i < selectedRows.size(); ++i) {
                    int row = selectedRows[i];
                    if (row >= 0 && row < processor.getSampleManager().getNumSamples()) {
                        // Use the proper method to remove a sample from its group
                        processor.getSampleManager().removeSampleFromGroup(row);
                    }
                }

                // Refresh the list
                updateContent();
            }
            break;
        }

        case CommandIDs::RemoveGroups: {
            // Remove all groups
            for (int i = processor.getSampleManager().getNumGroups() - 1; i >= 0; --i)
                processor.getSampleManager().removeGroup(i);

            // Refresh the list
            updateContent();
            break;
        }
    }
}

void SampleList::toggleOnsetRandomization(int sampleIndex) {
    if (sampleIndex >= 0 && sampleIndex < processor.getSampleManager().getNumSamples()) {
        auto *sound = processor.getSampleManager().getSampleSound(sampleIndex);
        if (sound != nullptr) {
            // Check if the sample has any onset markers
            if (sound->getOnsetMarkers().empty()) {
                // No onset markers available, show warning
                juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "No Onset Markers",
                        "This sample doesn't have any onset markers yet. Please edit the sample to detect or add onset markers.",
                        "OK",
                        this);
            } else {
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

void SampleList::toggleReverbForSample(int sampleIndex) {
    if (sampleIndex >= 0 && sampleIndex < processor.getSampleManager().getNumSamples()) {
        auto *sound = processor.getSampleManager().getSampleSound(sampleIndex);
        if (sound != nullptr) {
            // Toggle reverb enabled state
            bool newState = !sound->isReverbEnabled();
            sound->setReverbEnabled(newState);

            // Update content to refresh the icon
            updateContent();
        }
    }
}