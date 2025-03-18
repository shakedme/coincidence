#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "EnvelopePointManager.h"
#include "EnvelopeGridSystem.h"

/**
 * Handles rendering of envelope points, lines, curves, and selection areas
 */
class EnvelopeRenderer {
public:
    explicit EnvelopeRenderer(EnvelopePointManager &pointManager);

    ~EnvelopeRenderer() = default;

    void drawEnvelope(juce::Graphics &g, int width, int height, float transportPosition);

    void drawEnvelopeLine(juce::Graphics &g, int width, int height);

    void drawPoints(juce::Graphics &g, int width, int height);

    void drawSelectionArea(juce::Graphics &g, const juce::Rectangle<float> &area);

    void drawPositionMarker(juce::Graphics &g, int width, int height, float transportPosition);

private:
    EnvelopePointManager &pointManager;

    // Styling properties
    juce::Colour envelopeColor = juce::Colour(0xff52bfd9);
    juce::Colour selectedPointColor = juce::Colours::white;
    juce::Colour pointColor = juce::Colour(0xff52bfd9);
    juce::Colour positionMarkerColor = juce::Colours::white;
    juce::Colour selectionFillColor = juce::Colour(0x3052bfd9); // 30% opacity
    juce::Colour selectionOutlineColor = juce::Colour(0xff52bfd9);

    float pointRadius = 6.0f;
}; 