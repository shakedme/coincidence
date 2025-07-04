//
// Created by Shaked Melman on 01/03/2025.
//

#ifndef JUCECMAKEREPO_GROOVESECTION_H
#define JUCECMAKEREPO_GROOVESECTION_H

#include "BaseSection.h"
#include "../../Shared/Models.h"
#include "../Components/DirectionSelector.h"

class GrooveSectionComponent : public BaseSectionComponent
{
public:
    GrooveSectionComponent(PluginEditor& editor, PluginProcessor& processor);
    ~GrooveSectionComponent() override;

    void resized() override;

    // Update rate labels based on rhythm mode
    void updateRateLabelsForRhythmMode();

    // Repaint controls with randomization values
    void repaintRandomizationControls();

private:
    // UI Components
    std::array<std::unique_ptr<juce::Slider>, Models::NUM_RATE_OPTIONS> rateKnobs;
    std::array<std::unique_ptr<juce::Label>, Models::NUM_RATE_OPTIONS> rateLabels;

    std::unique_ptr<juce::ComboBox> rhythmModeComboBox;
    std::unique_ptr<juce::Label> rhythmModeLabel;

    std::unique_ptr<juce::Slider> probabilityKnob;
    std::unique_ptr<juce::Label> probabilityLabel;

    std::unique_ptr<juce::Slider> gateKnob;
    std::unique_ptr<juce::Slider> gateRandomKnob;
    std::unique_ptr<juce::Label> gateLabel;
    std::unique_ptr<juce::Label> gateRandomLabel;

    std::unique_ptr<juce::Slider> velocityKnob;
    std::unique_ptr<juce::Slider> velocityRandomKnob;
    std::unique_ptr<juce::Label> velocityLabel;
    std::unique_ptr<juce::Label> velocityRandomLabel;

    std::unique_ptr<DirectionSelector> gateDirectionSelector;
    std::unique_ptr<DirectionSelector> velocityDirectionSelector;

    // Setup methods
    void setupRateControls();
    void setupRhythmModeControls();
    void setupDensityControls();
    void setupGateControls();
    void setupVelocityControls();
    void setupDirectionControls();
};


#endif //JUCECMAKEREPO_GROOVESECTION_H
