//
// Created by Shaked Melman on 01/03/2025.
//

#ifndef JUCECMAKEREPO_GLITCHSECTION_H
#define JUCECMAKEREPO_GLITCHSECTION_H

#include "BaseSection.h"

class EffectsSection : public BaseSectionComponent
{
public:
    EffectsSection(PluginEditor& editor, PluginProcessor& processor);
    ~EffectsSection() override;

    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    // UI Components
    std::unique_ptr<juce::Slider> stutterKnob;
    std::unique_ptr<juce::Label> stutterLabel;
    
    // Reverb UI Components
    std::unique_ptr<juce::Slider> reverbMixKnob;
    std::unique_ptr<juce::Slider> reverbProbabilityKnob;
    std::unique_ptr<juce::Slider> reverbTimeKnob;
    std::unique_ptr<juce::Label> reverbMixLabel;
    std::unique_ptr<juce::Label> reverbProbabilityLabel;
    std::unique_ptr<juce::Label> reverbTimeLabel;
    std::unique_ptr<juce::Label> reverbSectionLabel;

    std::unique_ptr<juce::Slider> reverbDampingKnob;
    std::unique_ptr<juce::Label> reverbDampingLabel;

    std::unique_ptr<juce::Slider> reverbWidthKnob;
    std::unique_ptr<juce::Label> reverbWidthLabel;
};

#endif //JUCECMAKEREPO_GLITCHSECTION_H
