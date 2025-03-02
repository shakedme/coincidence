//
// Created by Shaked Melman on 01/03/2025.
//

#ifndef JUCECMAKEREPO_SAMPLESECTION_H
#define JUCECMAKEREPO_SAMPLESECTION_H

#include "BaseSection.h"

class SampleSectionComponent : public BaseSectionComponent,
                               public juce::TableListBoxModel,
                               public juce::FileDragAndDropTarget
{
public:
    SampleSectionComponent(PluginEditor& editor, PluginProcessor& processor);
    ~SampleSectionComponent() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    // TableListBoxModel overrides
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    void cellClicked(int rowNumber, int columnId, const juce::MouseEvent&) override;

    // FileDragAndDropTarget overrides
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;

private:
    // UI Components
    std::unique_ptr<juce::TableListBox> sampleListBox;
    std::unique_ptr<juce::TextButton> addSampleButton;
    std::unique_ptr<juce::TextButton> removeSampleButton;
    std::unique_ptr<juce::ToggleButton> randomizeToggle;
    std::unique_ptr<juce::Slider> randomizeProbabilitySlider;
    std::unique_ptr<juce::Label> randomizeProbabilityLabel;
    std::unique_ptr<juce::Label> sampleNameLabel;

    bool isCurrentlyOver = false;
};
#endif //JUCECMAKEREPO_SAMPLESECTION_H
