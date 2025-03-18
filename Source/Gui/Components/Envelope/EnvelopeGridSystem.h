#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Manages grid and snap-to-grid functionality for envelope
 */
class EnvelopeGridSystem {
public:
    EnvelopeGridSystem();

    // Draw the grid on the given graphics context
    void drawGrid(juce::Graphics &g, int width, int height, bool snapEnabled);

    // Set grid divisions
    void setGridDivisions(int horizontal, int vertical);

    // Set snap-to-grid enabled/disabled
    void setSnapToGridEnabled(bool enabled);

    // Check if snap-to-grid is enabled
    [[nodiscard]] bool isSnapToGridEnabled() const;

    // Snap a point to the grid if snap is enabled
    [[nodiscard]] juce::Point<float> snapToGrid(const juce::Point<float> &point) const;

private:
    // Grid divisions
    int horizontalDivisions = 10;
    int verticalDivisions = 4;

    // Snap to grid flag
    bool snapToGridEnabled = true;
}; 