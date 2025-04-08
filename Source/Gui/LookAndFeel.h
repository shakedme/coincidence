#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

// Forward declarations
class PluginProcessor;

/**
 * Custom look and feel for the plugin UI
 */
class LookAndFeel : public juce::LookAndFeel_V4 {
public:
    LookAndFeel();

    void drawRotarySlider(juce::Graphics &g,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPos,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider &slider) override;

    void drawComboBox(juce::Graphics &g,
                      int width,
                      int height,
                      bool,
                      int,
                      int,
                      int,
                      int,
                      juce::ComboBox &box) override;

    void drawLabel(juce::Graphics &g, juce::Label &label) override;

    void drawButtonBackground(juce::Graphics &g,
                              juce::Button &button,
                              const juce::Colour &backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    // Custom tab button drawing
    void drawTabButton(juce::TabBarButton &button,
                       juce::Graphics &g,
                       bool isMouseOver,
                       bool isMouseDown) override;
};
