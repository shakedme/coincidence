#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../../Audio/PluginProcessor.h"
#include "Icon.h"
#include "BinaryData.h"

class SampleList
    : public juce::Component
    , public juce::TableListBoxModel
{
public:
    explicit SampleList(PluginProcessor& processor);
    ~SampleList() override = default;

    PluginProcessor& processor;

    // TableListBoxModel overrides
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g,
                            int rowNumber,
                            int width,
                            int height,
                            bool rowIsSelected) override;
    void paintCell(juce::Graphics& g,
                   int rowNumber,
                   int columnId,
                   int width,
                   int height,
                   bool rowIsSelected) override;
    void deleteKeyPressed(int currentSelectedRow) override;
    void cellClicked(int rowNumber, int columnId, const juce::MouseEvent& e) override;
    juce::Component*
        refreshComponentForCell(int rowNumber,
                                int columnId,
                                bool isRowSelected,
                                juce::Component* existingComponentToUpdate) override;

    // Component overrides
    void resized() override;

    // Update content when samples change
    void updateContent();

    // Callback for when user clicks on edit/detail button
    std::function<void(int)> onSampleDetailRequested;

    // Get the table list box for keyboard focus
    juce::TableListBox* getListBox() { return sampleListBox.get(); }

    // Update the active sample highlight
    void setActiveSampleIndex(int index);

    // Called by the slider cell when its value changes
    void handleSliderValueChanged(int rowNumber, double value);

    // Menu handling
    void menuItemSelected(int menuItemID, int topLevelMenuIndex);

    // Toggle onset randomization for a sample
    void toggleOnsetRandomization(int sampleIndex);

    // Toggle reverb enabled for a sample
    void toggleReverbForSample(int sampleIndex);

private:
    std::unique_ptr<juce::TableListBox> sampleListBox;

    // Track the currently active sample for highlighting
    int activeSampleIndex = -1;

    // Constants for icon drawing
    const int ICON_SIZE = 16;
    const int ICON_PADDING = 8;

    // Context menu commands
    enum CommandIDs
    {
        GroupSelected = 1,
        UngroupSelected,
        RemoveGroups
    };

    // Get the color for a specific group
    juce::Colour getGroupColor(int groupIndex) const
    {
        // Different colors for each group (match with GroupListView colors)
        static const juce::Colour colors[] = {
            juce::Colour(0xff5c9ce6), // Blue
            juce::Colour(0xff52bf5d), // Green
            juce::Colour(0xffbf5252), // Red
            juce::Colour(0xffbf52d9) // Purple
        };

        // Make sure index is within range
        const int numColors = sizeof(colors) / sizeof(colors[0]);
        if (groupIndex >= 0 && groupIndex < numColors)
            return colors[groupIndex];

        return juce::Colours::grey;
    }
};

class ProbabilitySliderCell
    : public juce::Component
    , private juce::Slider::Listener
{
public:
    ProbabilitySliderCell(SampleList* owner, int rowNumber)
        : ownerList(owner)
        , row(rowNumber)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        slider.setRange(0.0, 1.0, 0.01);
        slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffbf52d9));
        slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffbf52d9));
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff444444));
        slider.setColour(juce::Slider::trackColourId,
                         juce::Colour(0xff222222)); // Darker track color
        slider.setColour(juce::Slider::backgroundColourId,
                         juce::Colour(0xff666666)); // Lighter background

        // Use the slider listener pattern instead of onValueChange
        slider.addListener(this);
        
        // Set a property to identify the slider for event handling
        slider.getProperties().set("slider", true);

        addAndMakeVisible(slider);
    }

    // Update the row number when component is reused
    void updateRow(int newRow)
    {
        row = newRow;
    }

    void resized() override
    {
        // Center the knob vertically and place it at the right side of the cell
        int knobSize = juce::jmin(getHeight() - 4, 32);
        int rightPadding = 10;
        
        slider.setBounds(getWidth() - knobSize - rightPadding, 
                         (getHeight() - knobSize) / 2, 
                         knobSize, 
                         knobSize);
    }

    void setValue(double value, juce::NotificationType notification)
    {
        slider.setValue(value, notification);
    }

    void sliderValueChanged(juce::Slider* sliderThatChanged) override
    {
        if (sliderThatChanged == &slider && ownerList != nullptr)
        {
            ownerList->handleSliderValueChanged(row, slider.getValue());
        }
    }
    
    void mouseDown(const juce::MouseEvent& e) override
    {
        // Only forward mouse events if not interacting with the slider
        if (!e.eventComponent->getProperties().contains("slider"))
        {
            // Forward the event to the parent component for selection handling
            if (juce::Component* parent = getParentComponent())
                parent->mouseDown(e.getEventRelativeTo(parent));
        }
    }
    
    void mouseUp(const juce::MouseEvent& e) override
    {
        // Only forward mouse events if not interacting with the slider
        if (!e.eventComponent->getProperties().contains("slider"))
        {
            // Forward the event to the parent component
            if (juce::Component* parent = getParentComponent())
                parent->mouseUp(e.getEventRelativeTo(parent));
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        // Only forward mouse events if not interacting with the slider
        if (!e.eventComponent->getProperties().contains("slider"))
        {
            // For drag selection, we need to forward this to the table list box
            if (juce::Component* parent = getParentComponent())
            {
                auto parentEvent = e.getEventRelativeTo(parent);
                
                // Find table list box parent component
                auto* tableComp = dynamic_cast<juce::TableListBox*>(parent->getParentComponent());
                if (tableComp != nullptr)
                {
                    // If using Shift or Ctrl/Cmd, extend selection to the row under mouse
                    if (e.mods.isShiftDown() || e.mods.isCommandDown() || e.mods.isCtrlDown())
                    {
                        // Get row index under current mouse position
                        auto relPos = parentEvent.getPosition();
                        int rowUnderMouse = tableComp->getRowContainingPosition(relPos.x, relPos.y);
                        
                        if (rowUnderMouse >= 0)
                        {
                            // If shift is down, select range
                            if (e.mods.isShiftDown())
                            {
                                // Get the anchor (first selected row)
                                auto selectedRows = tableComp->getSelectedRows();
                                int anchorRow = selectedRows.isEmpty() ? row : selectedRows[0];
                                
                                // Select range from anchor to current row
                                tableComp->selectRangeOfRows(juce::jmin(anchorRow, rowUnderMouse), 
                                                          juce::jmax(anchorRow, rowUnderMouse));
                            }
                            // If Ctrl/Cmd is down, add to selection
                            else if (e.mods.isCommandDown() || e.mods.isCtrlDown())
                            {
                                tableComp->selectRow(rowUnderMouse, true); // Add to selection
                            }
                        }
                    }
                }
                
                // Forward the drag event
                parent->mouseDrag(parentEvent);
            }
        }
    }

private:
    juce::Slider slider;
    SampleList* ownerList;
    int row;
};