//
// Created by Shaked Melman on 01/03/2025.
//

#ifndef JUCECMAKEREPO_BASESECTION_H
#define JUCECMAKEREPO_BASESECTION_H

#include "juce_audio_utils/juce_audio_utils.h"
#include "../../Audio/MidiGeneratorProcessor.h"

class BaseSectionComponent : public juce::Component
{
public:
    BaseSectionComponent(MidiGeneratorEditor& editor,
                         MidiGeneratorProcessor& processor,
                         const juce::String& title,
                         juce::Colour colour);

    virtual ~BaseSectionComponent() override = default;

    virtual void resized() override = 0;
    virtual void paint(juce::Graphics& g) override;

    const juce::String& getSectionTitle() const { return sectionTitle; }
    juce::Colour getSectionColour() const { return sectionColour; }

protected:
    MidiGeneratorEditor& editor;
    MidiGeneratorProcessor& processor;
    juce::String sectionTitle;
    juce::Colour sectionColour;
    std::unique_ptr<juce::Label> sectionLabel;

    // Helper methods
    juce::Label* createLabel(const juce::String& text, juce::Justification justification = juce::Justification::centred);
    juce::Slider* createRotarySlider(const juce::String& tooltip);

    // Parameter attachments
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboBoxAttachments;
};


#endif //JUCECMAKEREPO_BASESECTION_H
