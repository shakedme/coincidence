#pragma once

#include <vector>
#include <memory>
#include <functional>
#include "EnvelopePoint.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Manages envelope points and their interaction
 */
class EnvelopePointManager {
public:
    explicit EnvelopePointManager(
            int horizontalDivisions = 10,
            int verticalDivisions = 4
    );

    ~EnvelopePointManager() = default;

    void addPoint(float x, float y, bool editable = true);

    bool removePoint(int index);

    void clearSelectedPoints();

    void selectPoint(int index);

    void deselectAllPoints();

    void selectPointsInArea(const juce::Rectangle<float> &area);

    [[nodiscard]] int getSelectedPointsCount() const;

    // Point finding
    [[nodiscard]] int findPointAt(const juce::Point<float> &position, float radius) const;

    [[nodiscard]] int
    findClosestSegmentIndex(const juce::Point<float> &position, float threshold = 10.0f) const;

    // Point movement
    void movePoint(int index, float x, float y);

    void moveSelectedPoints(float deltaX, float deltaY);

    // Curve handling
    void setCurvature(int segmentIndex, float curvature);

    [[nodiscard]] float getCurvature(int segmentIndex) const;

    // Drawing
    [[nodiscard]] juce::Point<float> getPointScreenPosition(int index) const;

    [[nodiscard]] juce::Point<float> getPointScreenPosition(const EnvelopePoint &point) const;

    // Point utility functions
    [[nodiscard]] float
    distanceToLineSegment(const juce::Point<float> &p, const juce::Point<float> &v, const juce::Point<float> &w) const;

    [[nodiscard]] float
    distanceToCurve(const juce::Point<float> &point, const juce::Point<float> &start, const juce::Point<float> &end,
                    float curvature) const;

    [[nodiscard]] const std::vector<std::unique_ptr<EnvelopePoint>> &getPoints() const;

    void setPoints(std::vector<std::unique_ptr<EnvelopePoint>> newPoints);

    void sortPoints();

    void setBounds(int width, int height);

    [[nodiscard]] juce::Point<float> snapToGrid(const juce::Point<float> &point) const;

    // Callback for point changes
    std::function<void()> onPointsChanged;

private:
    int horizontalDivisions = 10;
    int verticalDivisions = 4;

    int width;
    int height;

    std::vector<std::unique_ptr<EnvelopePoint>> points;

    void notifyPointsChanged();

    static constexpr float pointRadius = 6.0f;
}; 