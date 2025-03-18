#include "EnvelopeGridSystem.h"

EnvelopeGridSystem::EnvelopeGridSystem()
    : horizontalDivisions(10), verticalDivisions(4), snapToGridEnabled(true)
{
}

void EnvelopeGridSystem::drawGrid(juce::Graphics& g, int width, int height, bool snapEnabled)
{
    // Draw standard grid
    g.setColour(juce::Colour(0xff444444));

    // Draw vertical lines
    for (int i = 0; i <= horizontalDivisions; ++i) {
        const float x = i * (width / horizontalDivisions);
        g.drawLine(x, 0, x, height, 1.0f);
    }

    // Draw horizontal lines
    for (int i = 0; i <= verticalDivisions; ++i) {
        const float y = i * (height / verticalDivisions);
        g.drawLine(0, y, width, y, 1.0f);
    }

    // Draw center line (value 50%)
    g.setColour(juce::Colour(0xff666666));
    g.drawLine(0, height / 2, width, height / 2, 1.5f);
    
    // If snap to grid is enabled, make grid slightly brighter
    if (snapEnabled) {
        g.setColour(juce::Colour(0xff888888));
        
        // Draw vertical snap lines
        for (int i = 0; i <= horizontalDivisions; ++i) {
            const float x = i * (width / horizontalDivisions);
            g.drawLine(x, 0, x, height, 0.5f);
        }
        
        // Draw horizontal snap lines
        for (int i = 0; i <= verticalDivisions; ++i) {
            const float y = i * (height / verticalDivisions);
            g.drawLine(0, y, width, y, 0.5f);
        }
    }
}

void EnvelopeGridSystem::setGridDivisions(int horizontal, int vertical)
{
    horizontalDivisions = horizontal;
    verticalDivisions = vertical;
}

void EnvelopeGridSystem::setSnapToGridEnabled(bool enabled)
{
    snapToGridEnabled = enabled;
}

bool EnvelopeGridSystem::isSnapToGridEnabled() const
{
    return snapToGridEnabled;
}

juce::Point<float> EnvelopeGridSystem::snapToGrid(const juce::Point<float>& point) const
{
    if (!snapToGridEnabled)
        return point;
    
    // Define threshold distance for snapping (as a fraction of grid cell size)
    const float snapThresholdX = 0.2f / horizontalDivisions;
    const float snapThresholdY = 0.2f / verticalDivisions;
    
    // Calculate grid steps
    float gridStepX = 1.0f / horizontalDivisions;
    float gridStepY = 1.0f / verticalDivisions;
    
    // Initialize result with original point
    float snappedX = point.x;
    float snappedY = point.y;
    
    // Check if point is close to a horizontal grid line
    float xMod = std::fmod(point.x, gridStepX);
    if (xMod < snapThresholdX) {
        // Snap to lower grid line
        snappedX = std::floor(point.x / gridStepX) * gridStepX;
    } else if (gridStepX - xMod < snapThresholdX) {
        // Snap to upper grid line
        snappedX = std::ceil(point.x / gridStepX) * gridStepX;
    }
    
    // Check if point is close to a vertical grid line
    float yMod = std::fmod(point.y, gridStepY);
    if (yMod < snapThresholdY) {
        // Snap to lower grid line
        snappedY = std::floor(point.y / gridStepY) * gridStepY;
    } else if (gridStepY - yMod < snapThresholdY) {
        // Snap to upper grid line
        snappedY = std::ceil(point.y / gridStepY) * gridStepY;
    }
    
    // Ensure values are within 0-1 range
    snappedX = juce::jlimit(0.0f, 1.0f, snappedX);
    snappedY = juce::jlimit(0.0f, 1.0f, snappedY);
    
    return juce::Point<float>(snappedX, snappedY);
} 