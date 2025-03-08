#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

/**
 * A generic icon component that loads and displays an image from a file.
 * Supports tooltips and click handling.
 */// Modify your Icon class like this:

class Icon : public juce::Component, public juce::SettableTooltipClient
{
public:
    Icon(const char* svgData, int svgDataSize, float size = 16.0f)
    {
        setInterceptsMouseClicks(true, false);
        setSize(size, size);

        // Create Drawable directly from SVG data
        if (svgData != nullptr && svgDataSize > 0)
        {
            auto xml = juce::XmlDocument::parse(juce::String::createStringFromData(svgData, svgDataSize));
            if (xml != nullptr)
            {
                drawable = juce::Drawable::createFromSVG(*xml);
            }

            // Debug output
            DBG("SVG parsed: " + juce::String(drawable != nullptr ? "yes" : "no"));
        }
    }

    void paint(juce::Graphics& g) override
    {
        // Apply current color to drawable
        if (drawable != nullptr)
        {
            // Apply current color
            juce::Colour currentColor = isEnabled() ?
                                                    (isActive ? activeColour : normalColour) :
                                                    juce::Colours::darkgrey;

            if (isMouseOver())
                currentColor = currentColor.brighter(0.2f);

            if (isMouseButtonDown())
                currentColor = currentColor.brighter(0.5f);

            // Set the fill color on the drawable
            drawable->replaceColour(juce::Colours::black, currentColor);

            // Draw the SVG
            drawable->drawWithin(g, getLocalBounds().toFloat(),
                                 juce::RectanglePlacement::centred, 1.0f);
        }
    }

    // Mouse event handlers remain the same
    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override { repaint(); }
    void mouseDown(const juce::MouseEvent&) override { repaint(); }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (e.getNumberOfClicks() > 0 && contains(e.getPosition()) && onClicked)
            onClicked();
        repaint();
    }

    void setActive(bool shouldBeActive, juce::Colour colour = juce::Colour(0xff52bfd9))
    {
        isActive = shouldBeActive;
        activeColour = colour;
        repaint();
    }

    void setNormalColour(juce::Colour colour)
    {
        normalColour = colour;
        repaint();
    }

    // Callback for when icon is clicked
    std::function<void()> onClicked;

private:
    std::unique_ptr<juce::Drawable> drawable;
    juce::Colour normalColour = juce::Colours::lightgrey;
    juce::Colour activeColour = juce::Colour(0xff52bfd9);
    bool isActive = false;
};