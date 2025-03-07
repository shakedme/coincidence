//
// Created by Shaked Melman on 01/03/2025.
//

#ifndef JUCECMAKEREPO_BASESECTION_H
#define JUCECMAKEREPO_BASESECTION_H

#include "juce_audio_utils/juce_audio_utils.h"
#include "../../Audio/PluginProcessor.h"
#include "../LookAndFeel.h"

static constexpr int HEADER_HEIGHT = 30;
static constexpr int TITLE_FONT_SIZE = 20;


class BaseSectionComponent : public juce::Component
{
public:
    BaseSectionComponent(PluginEditor& editor,
                         PluginProcessor& processor,
                         const juce::String& title,
                         juce::Colour colour);

    virtual ~BaseSectionComponent() override
    {
    }
    virtual void resized() override = 0;
    virtual void paint(juce::Graphics& g) override;

    const juce::String& getSectionTitle() const { return sectionTitle; }
    juce::Colour getSectionColour() const { return sectionColour; }

protected:
    PluginEditor& editor;
    PluginProcessor& processor;
    juce::String sectionTitle;
    juce::Colour sectionColour;
    std::unique_ptr<juce::Label> sectionLabel;

    // Parameter attachments
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>
        sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>>
        buttonAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>>
        comboBoxAttachments;

    int firstRowY = 45;

    // Helper methods
    juce::Label*
        createLabel(const juce::String& text,
                    juce::Justification justification = juce::Justification::centred);
    juce::Slider* createRotarySlider(const juce::String& tooltip);
    void drawMetallicPanel(juce::Graphics& g);
    void clearAttachments();
    void initKnob(
        std::unique_ptr<juce::Slider>& knob,
        const juce::String& tooltip,
        const juce::String& name,
        int min = 0,
        int max = 100,
        double interval = 0.1,
        const juce::String& textSuffix = "%"
        );
    void initLabel(
        std::unique_ptr<juce::Label>& label,
        const juce::String& text,
        juce::Justification justification = juce::Justification::centred,
        float fontSize = 11.0f
        );
};

#endif //JUCECMAKEREPO_BASESECTION_H
