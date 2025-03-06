#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../../Audio/PluginProcessor.h"

// Forward declare the custom cell components
class ProbabilitySliderCell;
class OnsetToggleButtonCell;

class SampleList
    : public juce::Component
    , public juce::TableListBoxModel
{
public:
    explicit SampleList(PluginProcessor& processor);
    ~SampleList() override = default;

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
    
    // Called by the onset toggle button when its value changes
    void handleOnsetToggleChanged(int rowNumber, bool toggleState);

    // Menu handling
    void menuItemSelected(int menuItemID, int topLevelMenuIndex);

private:
    PluginProcessor& processor;
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
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        slider.setRange(0.0, 1.0, 0.01);
        slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffbf52d9));
        slider.setColour(juce::Slider::trackColourId,
                         juce::Colour(0xff222222)); // Darker track color
        slider.setColour(juce::Slider::backgroundColourId,
                         juce::Colour(0xff666666)); // Lighter background

        // Use the slider listener pattern instead of onValueChange
        slider.addListener(this);

        addAndMakeVisible(slider);
    }

    void resized() override
    {
        // Position the slider with fixed width (70% of container)
        int sliderWidth =
            juce::jlimit(50, getWidth() - 10, static_cast<int>(getWidth() * 0.7f));
        int sliderX = (getWidth() - sliderWidth) / 2;

        slider.setBounds(sliderX, 2, sliderWidth, getHeight() - 4);
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

private:
    juce::Slider slider;
    SampleList* ownerList;
    int row;
};

// Custom component for the Use Onsets toggle button
class OnsetToggleButtonCell
    : public juce::Component
    , private juce::Button::Listener
{
public:
    OnsetToggleButtonCell(SampleList* owner, int rowNumber)
        : ownerList(owner)
        , row(rowNumber)
    {
        toggleButton.setButtonText("Beat Slice");
        toggleButton.setTooltip("Enable beat onset playback mode");
        toggleButton.addListener(this);
        addAndMakeVisible(toggleButton);
    }

    ~OnsetToggleButtonCell() override = default;

    void resized() override
    {
        toggleButton.setBounds(getLocalBounds().reduced(2));
    }

    void setState(bool state, juce::NotificationType notification)
    {
        toggleButton.setToggleState(state, notification);
    }

private:
    void buttonClicked(juce::Button* button) override
    {
        if (button == &toggleButton && ownerList != nullptr)
        {
            ownerList->handleOnsetToggleChanged(row, toggleButton.getToggleState());
        }
    }

    SampleList* ownerList;
    int row;
    juce::ToggleButton toggleButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OnsetToggleButtonCell)
};