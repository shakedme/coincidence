#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "EnvelopePointManager.h"

/**
 * Handles rendering of envelope points, lines, curves, and selection areas
 */
class EnvelopeRenderer {
public:
    explicit EnvelopeRenderer(
            EnvelopePointManager &pointManager,
            int horizontalDivisions = 10,
            int verticalDivisions = 4
    );

    ~EnvelopeRenderer() = default;

    void drawEnvelope(juce::Graphics &g, float transportPosition);

    void drawEnvelopeLine(juce::Graphics &g);

    void drawPoints(juce::Graphics &g);

    void drawSelectionArea(juce::Graphics &g, const juce::Rectangle<float> &area);

    void drawPositionMarker(juce::Graphics &g, float transportPosition);

    void drawGrid(juce::Graphics &g);

    void setBounds(int width, int height) {
        this->width = width;
        this->height = height;
    }

private:
    EnvelopePointManager &pointManager;

    int horizontalDivisions = 10;
    int verticalDivisions = 4;
    int height;
    int width;

    juce::Colour envelopeColor = juce::Colour(0xff52bfd9);
    juce::Colour selectedPointColor = juce::Colours::white;
    juce::Colour pointColor = juce::Colour(0xff52bfd9);
    juce::Colour positionMarkerColor = juce::Colours::white;
    juce::Colour selectionFillColor = juce::Colour(0x3052bfd9); // 30% opacity
    juce::Colour selectionOutlineColor = juce::Colour(0xff52bfd9);

    float pointRadius = 6.0f;
}; 