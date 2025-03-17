//
// Created by Shaked Melman on 01/03/2025.
//

#ifndef JUCECMAKEREPO_GLITCHSECTION_H
#define JUCECMAKEREPO_GLITCHSECTION_H

#include "BaseSection.h"
#include "../Components/Toggle.h"

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
    std::unique_ptr<juce::Label> stutterSectionLabel;
    
    // Reverb UI Components
    std::unique_ptr<juce::Slider> reverbMixKnob;
    std::unique_ptr<juce::Slider> reverbTimeKnob;
    std::unique_ptr<juce::Label> reverbMixLabel;
    std::unique_ptr<juce::Label> reverbTimeLabel;
    std::unique_ptr<juce::Label> reverbSectionLabel;

    std::unique_ptr<juce::Slider> reverbDampingKnob;
    std::unique_ptr<juce::Label> reverbDampingLabel;

    std::unique_ptr<juce::Slider> reverbWidthKnob;
    std::unique_ptr<juce::Label> reverbWidthLabel;
    
    // Delay UI Components
    std::unique_ptr<juce::Slider> delayMixKnob;
    std::unique_ptr<juce::Slider> delayRateKnob;
    std::unique_ptr<juce::Slider> delayFeedbackKnob;
    std::unique_ptr<Toggle> delayPingPongToggle;
    std::unique_ptr<Toggle> delayBpmSyncToggle;
    std::unique_ptr<juce::Label> delayMixLabel;
    std::unique_ptr<juce::Label> delayRateLabel;
    std::unique_ptr<juce::Label> delayFeedbackLabel;
    
    std::unique_ptr<juce::Label> delaySectionLabel;
    
    // Helper methods
    void updateDelayRateKnobTooltip();
    void updatePingPongTooltip();
    void updateBpmSyncTooltip();
    
    // Parameter attachments for sliders
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
};

#endif //JUCECMAKEREPO_GLITCHSECTION_H
