#pragma once

#include <vector>
#include <memory>
#include <functional>
#include "../../../Shared/Envelope/EnvelopePoint.h"
#include "EnvelopeGridSystem.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Manages envelope points and their interaction
 */
class EnvelopePointManager {
public:
    explicit EnvelopePointManager(EnvelopeGridSystem &gridSystem);

    ~EnvelopePointManager() = default;

    // Point management
    void addPoint(float x, float y, bool editable = true);

    bool removePoint(int index);

    void clearSelectedPoints();

    void selectPoint(int index);

    void deselectAllPoints();

    void selectPointsInArea(const juce::Rectangle<float> &area, int componentWidth = 800, int componentHeight = 600);

    int getSelectedPointsCount() const;

    // Point finding
    int findPointAt(const juce::Point<float> &position, float radius, int componentWidth = 800,
                    int componentHeight = 600) const;

    int findClosestSegmentIndex(const juce::Point<float> &position, float threshold = 10.0f, int componentWidth = 800,
                                int componentHeight = 600) const;

    // Point movement
    void movePoint(int index, float x, float y);

    void moveSelectedPoints(float deltaX, float deltaY);

    // Curve handling
    void setCurvature(int segmentIndex, float curvature);

    float getCurvature(int segmentIndex) const;

    // Drawing
    juce::Point<float> getPointScreenPosition(int index, int width, int height) const;

    juce::Point<float> getPointScreenPosition(const EnvelopePoint &point, int width, int height) const;

    // Point utility functions
    float
    distanceToLineSegment(const juce::Point<float> &p, const juce::Point<float> &v, const juce::Point<float> &w) const;

    float
    distanceToCurve(const juce::Point<float> &point, const juce::Point<float> &start, const juce::Point<float> &end,
                    float curvature) const;

    // Access points
    const std::vector<std::unique_ptr<EnvelopePoint>> &getPoints() const;

    void setPoints(std::vector<std::unique_ptr<EnvelopePoint>> newPoints);

    void sortPoints();

    // Callback for point changes
    std::function<void()> onPointsChanged;

private:
    std::vector<std::unique_ptr<EnvelopePoint>> points;
    EnvelopeGridSystem &gridSystem;

    void notifyPointsChanged();

    static constexpr float pointRadius = 6.0f;
}; 