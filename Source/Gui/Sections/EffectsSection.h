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

private:
    // UI Components
    std::unique_ptr<juce::Slider> stutterKnob;
    std::unique_ptr<juce::Label> stutterLabel;
};

#endif //JUCECMAKEREPO_GLITCHSECTION_H
