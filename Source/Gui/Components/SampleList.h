#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../../Audio/PluginProcessor.h"

class SampleList : public juce::Component, public juce::TableListBoxModel
{
public:
    SampleList(PluginProcessor& processor);
    ~SampleList() override;

    // TableListBoxModel overrides
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    void deleteKeyPressed(int currentSelectedRow) override;
    void cellClicked(int rowNumber, int columnId, const juce::MouseEvent& e) override;

    // Component overrides
    void resized() override;
    void paint(juce::Graphics& g) override;

    // Update content when samples change
    void updateContent();
    
    // Callback for when user clicks on edit/detail button
    std::function<void(int)> onSampleDetailRequested;
    
    // Get the table list box for keyboard focus
    juce::TableListBox* getListBox() { return sampleListBox.get(); }

    // Update the active sample highlight
    void setActiveSampleIndex(int index);

private:
    PluginProcessor& processor;
    std::unique_ptr<juce::TableListBox> sampleListBox;

    // Track the currently active sample for highlighting
    int activeSampleIndex = -1;

    // Constants for icon drawing
    const int ICON_SIZE = 16;
    const int ICON_PADDING = 8;
};