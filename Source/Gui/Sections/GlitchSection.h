//
// Created by Shaked Melman on 01/03/2025.
//

#ifndef JUCECMAKEREPO_GLITCHSECTION_H
#define JUCECMAKEREPO_GLITCHSECTION_H

#include "BaseSection.h"

class GlitchSectionComponent : public BaseSectionComponent
{
public:
    GlitchSectionComponent(MidiGeneratorEditor& editor, MidiGeneratorProcessor& processor);
    ~GlitchSectionComponent() override = default;

    void resized() override;

private:
    // UI Components
    std::array<std::unique_ptr<juce::Slider>, 6> glitchKnobs;
    std::array<std::unique_ptr<juce::Label>, 6> glitchLabels;
};

#endif //JUCECMAKEREPO_GLITCHSECTION_H
