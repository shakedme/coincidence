//
// Created by Shaked Melman on 01/03/2025.
//

#ifndef JUCECMAKEREPO_PITCHSECTION_H
#define JUCECMAKEREPO_PITCHSECTION_H

#include "BaseSection.h"

class PitchSectionComponent : public BaseSectionComponent
{
public:
    PitchSectionComponent(PluginEditor& editor, PluginProcessor& processor);
    ~PitchSectionComponent() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    // UI Components
    std::unique_ptr<juce::ComboBox> scaleTypeComboBox;
    std::unique_ptr<juce::Label> scaleLabel;

    std::unique_ptr<juce::Slider> semitonesKnob;
    std::unique_ptr<juce::Slider> semitonesProbabilityKnob;
    std::unique_ptr<juce::Label> semitonesLabel;
    std::unique_ptr<juce::Label> semitonesProbabilityLabel;

    std::unique_ptr<juce::Slider> octavesKnob;
    std::unique_ptr<juce::Slider> octavesProbabilityKnob;
    std::unique_ptr<juce::Label> octavesLabel;
    std::unique_ptr<juce::Label> octavesProbabilityLabel;

    // Setup methods
    void setupScaleTypeControls();
    void setupSemitoneControls();
    void setupOctaveControls();
};

#endif //JUCECMAKEREPO_PITCHSECTION_H
