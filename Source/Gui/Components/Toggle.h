#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

class Toggle : public juce::Component, public juce::SettableTooltipClient
{
public:
    Toggle(juce::Colour activeColor, juce::Colour inactiveColor = juce::Colour(0xff808080))
        : activeColour(activeColor)
        , inactiveColour(inactiveColor)
    {
        setSize(60, 20);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        float cornerSize = bounds.getHeight() * 0.5f;

        // Draw background track
        g.setColour(juce::Colour(0xff3a3a3a));
        g.fillRoundedRectangle(bounds, cornerSize);

        // Draw border
        g.setColour(juce::Colour(0xff505050));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);

        // Calculate knob position
        float knobSize = bounds.getHeight() * 0.8f;
        float knobPadding = (bounds.getHeight() - knobSize) * 0.5f;
        float knobX = value ? bounds.getRight() - knobSize - knobPadding
                            : bounds.getX() + knobPadding;

        // Draw knob
        juce::Rectangle<float> knobBounds(
            knobX, bounds.getY() + knobPadding, knobSize, knobSize);

        // Create metallic gradient for knob using appropriate color based on state
        juce::Colour currentColor = value ? activeColour : inactiveColour;
        g.setGradientFill(juce::ColourGradient(currentColor.brighter(0.2f),
                                               knobBounds.getX(),
                                               knobBounds.getY(),
                                               currentColor.darker(0.2f),
                                               knobBounds.getRight(),
                                               knobBounds.getBottom(),
                                               true));
        g.fillEllipse(knobBounds);

        // Add highlight for 3D effect
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.fillEllipse(knobBounds.reduced(knobSize * 0.7f)
                          .translated(-knobSize * 0.1f, -knobSize * 0.1f));

        // Draw border around knob
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.drawEllipse(knobBounds, 1.0f);
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        // Toggle the value
        setValue(!value);

        // Notify listeners
        if (onValueChanged)
            onValueChanged(value);

        repaint();
    }

    void setValue(bool newValue)
    {
        if (value != newValue)
        {
            value = newValue;
            repaint();
        }
    }

    bool getValue() const { return value; }

    std::function<void(bool)> onValueChanged;

private:
    bool value = false;
    juce::Colour activeColour;
    juce::Colour inactiveColour;
};