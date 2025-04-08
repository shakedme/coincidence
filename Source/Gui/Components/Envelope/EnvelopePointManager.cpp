#include "EnvelopePointManager.h"
#include <algorithm>
#include <limits>

EnvelopePointManager::EnvelopePointManager(int hDivisions, int vDivisons)
        : horizontalDivisions(hDivisions), verticalDivisions(vDivisons) {
    addPoint(0.0f, 0.5f, false);
    addPoint(1.0f, 0.5f, false);
}

void EnvelopePointManager::addPoint(float x, float y, bool editable) {
    auto newPoint = std::make_unique<EnvelopePoint>(x, y, editable);

    juce::Point<float> snapped = snapToGrid({x, y});
    newPoint->position.setXY(snapped.x, snapped.y);

    auto it = std::upper_bound(points.begin(), points.end(), newPoint,
                               [](const auto &a, const auto &b) {
                                   return a->position.x < b->position.x;
                               });

    points.insert(it, std::move(newPoint));
    notifyPointsChanged();
}

bool EnvelopePointManager::removePoint(int index) {
    if (index <= 0 || index >= static_cast<int>(points.size()) - 1) {
        return false;
    }

    points.erase(points.begin() + index);
    notifyPointsChanged();
    return true;
}

void EnvelopePointManager::clearSelectedPoints() {
    // Remove all selected points except first and last
    points.erase(
            std::remove_if(points.begin() + 1, points.end() - 1,
                           [](const auto &point) {
                               return point->selected;
                           }),
            points.end() - 1
    );
    notifyPointsChanged();
}

void EnvelopePointManager::selectPoint(int index) {
    if (index >= 0 && index < static_cast<int>(points.size())) {
        points[index]->selected = true;
    }
}

void EnvelopePointManager::deselectAllPoints() {
    for (auto &point: points) {
        point->selected = false;
    }
}

void
EnvelopePointManager::selectPointsInArea(const juce::Rectangle<float> &area) {
    for (size_t i = 0; i < points.size(); ++i) {
        juce::Point<float> pos = getPointScreenPosition(i);

        if (area.contains(pos)) {
            points[i]->selected = true;
        }
    }
}

int EnvelopePointManager::getSelectedPointsCount() const {
    int count = 0;
    for (const auto &point: points) {
        if (point->selected) {
            count++;
        }
    }
    return count;
}

int EnvelopePointManager::findPointAt(const juce::Point<float> &position, float radius) const {
    for (size_t i = 0; i < points.size(); ++i) {
        juce::Point<float> pos = getPointScreenPosition(i);

        if (pos.getDistanceFrom(position) < radius) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

int
EnvelopePointManager::findClosestSegmentIndex(const juce::Point<float> &position, float threshold) const {
    int segmentIndex = -1;
    float minDistance = std::numeric_limits<float>::max();

    for (int i = 0; i < points.size() - 1; ++i) {
        juce::Point<float> p1 = getPointScreenPosition(i);
        juce::Point<float> p2 = getPointScreenPosition(i + 1);

        float dist;
        // If the segment has curvature, check distance to the curve
        if (points[i + 1]->curvature != 0.0f) {
            dist = distanceToCurve(position, p1, p2, points[i + 1]->curvature);
        } else {
            // For straight lines, use the original calculation
            dist = distanceToLineSegment(position, p1, p2);
        }

        if (dist < minDistance && dist < threshold) {
            minDistance = dist;
            segmentIndex = i;
        }
    }

    return segmentIndex;
}

void EnvelopePointManager::movePoint(int index, float x, float y) {
    if (index < 0 || index >= static_cast<int>(points.size())) {
        return;
    }

    float newX = x;
    float newY = y;

    // For first and last points, only allow vertical movement
    if (index == 0 || index == static_cast<int>(points.size()) - 1) {
        newX = points[index]->position.x;
    }

    // Apply snap to grid if enabled
    juce::Point<float> normalizedPos(newX, newY);
    juce::Point<float> snappedPos = snapToGrid(normalizedPos);

    // For first and last points, only apply vertical snapping
    if (index == 0 || index == static_cast<int>(points.size()) - 1) {
        newY = snappedPos.y;
    } else {
        newX = snappedPos.x;
        newY = snappedPos.y;
    }

    // Ensure values are within 0-1 range
    newX = juce::jlimit(0.0f, 1.0f, newX);
    newY = juce::jlimit(0.0f, 1.0f, newY);

    points[index]->position.setXY(newX, newY);

    // Sort points to maintain order based on x position
    sortPoints();
    notifyPointsChanged();
}

void EnvelopePointManager::moveSelectedPoints(float deltaX, float deltaY) {
    for (size_t i = 0; i < points.size(); ++i) {
        if (points[i]->selected) {
            float newX, newY;

            // For first and last points, only apply vertical movement
            if (i == 0 || i == points.size() - 1) {
                newX = points[i]->position.x; // Keep original X position
                newY = juce::jlimit(0.0f, 1.0f, points[i]->position.y + deltaY);
            } else {
                // Apply movement while keeping points within bounds
                newX = juce::jlimit(0.0f, 1.0f, points[i]->position.x + deltaX);
                newY = juce::jlimit(0.0f, 1.0f, points[i]->position.y + deltaY);
            }

            // Apply snap to grid if enabled
            juce::Point<float> normalizedPos(newX, newY);
            juce::Point<float> snappedPos = snapToGrid(normalizedPos);

            // For first and last points, only apply vertical snapping
            if (i == 0 || i == points.size() - 1) {
                newY = snappedPos.y;
            } else {
                newX = snappedPos.x;
                newY = snappedPos.y;
            }

            points[i]->position.setXY(newX, newY);
        }
    }

    // Sort points to maintain order based on x position
    sortPoints();
    notifyPointsChanged();
}

void EnvelopePointManager::setCurvature(int segmentIndex, float curvature) {
    if (segmentIndex >= 0 && segmentIndex < static_cast<int>(points.size()) - 1) {
        points[segmentIndex + 1]->curvature = juce::jlimit(-1.0f, 1.0f, curvature);
        notifyPointsChanged();
    }
}

float EnvelopePointManager::getCurvature(int segmentIndex) const {
    if (segmentIndex >= 0 && segmentIndex < static_cast<int>(points.size()) - 1) {
        return points[segmentIndex + 1]->curvature;
    }
    return 0.0f;
}

juce::Point<float> EnvelopePointManager::getPointScreenPosition(int index) const {
    if (index < 0 || index >= static_cast<int>(points.size())) {
        return {0.0f, 0.0f};
    }

    return getPointScreenPosition(*points[index]);
}

juce::Point<float>
EnvelopePointManager::getPointScreenPosition(const EnvelopePoint &point) const {
    // Convert normalized position to screen coordinates
    // Note that Y is inverted in GUI coordinates (0 is top)
    return juce::Point<float>(
            point.position.x * width,
            (1.0f - point.position.y) * height // Invert Y
    );
}

float EnvelopePointManager::distanceToLineSegment(const juce::Point<float> &p, const juce::Point<float> &v,
                                                  const juce::Point<float> &w) const {
    // Return minimum distance between line segment vw and point p
    const float l2 = v.getDistanceSquaredFrom(w);  // length squared of segment vw

    // If segment is a point, return distance to the point
    if (l2 == 0.0f)
        return p.getDistanceFrom(v);

    // Consider the line extending the segment, parameterized as v + t (w - v)
    // We find projection of point p onto the line.
    // Projection falls on the segment if t is between 0 and 1
    const float t = juce::jlimit(0.0f, 1.0f, ((p.x - v.x) * (w.x - v.x) + (p.y - v.y) * (w.y - v.y)) / l2);

    // Projection point on the line segment
    const juce::Point<float> projection = {v.x + t * (w.x - v.x), v.y + t * (w.y - v.y)};

    return p.getDistanceFrom(projection);
}

float EnvelopePointManager::distanceToCurve(const juce::Point<float> &point, const juce::Point<float> &start,
                                            const juce::Point<float> &end, float curvature) const {
    // Sample several points along the curve and find the closest one
    const int numSamples = 20;
    float minDistance = std::numeric_limits<float>::max();

    // Calculate the control point for the quadratic bezier
    // Use the same inversion as in drawEnvelopeLine for consistency
    const float curveAmount = -100.0f * curvature; // Invert direction to match visual display
    juce::Point<float> midPoint = start + (end - start) * 0.5f;
    juce::Point<float> perpendicular(-((end.y - start.y)), (end.x - start.x));

    // Normalize the perpendicular vector
    float length = std::sqrt(perpendicular.x * perpendicular.x + perpendicular.y * perpendicular.y);
    if (length > 0.0f) {
        perpendicular.x = perpendicular.x / length * curveAmount;
        perpendicular.y = perpendicular.y / length * curveAmount;
    }

    juce::Point<float> controlPoint = midPoint + perpendicular;

    // Sample points along the curve and find the closest one
    for (int i = 0; i <= numSamples; ++i) {
        float t = (float) i / numSamples;

        // Quadratic bezier formula: B(t) = (1-t)²P₀ + 2(1-t)tP₁ + t²P₂
        float oneMinusT = 1.0f - t;
        float oneMinusTSquared = oneMinusT * oneMinusT;
        float tSquared = t * t;

        juce::Point<float> samplePoint(
                oneMinusTSquared * start.x + 2.0f * oneMinusT * t * controlPoint.x + tSquared * end.x,
                oneMinusTSquared * start.y + 2.0f * oneMinusT * t * controlPoint.y + tSquared * end.y
        );

        float distance = point.getDistanceFrom(samplePoint);
        minDistance = std::min(minDistance, distance);
    }

    return minDistance;
}


juce::Point<float> EnvelopePointManager::snapToGrid(const juce::Point<float> &point) const {
    // Define threshold distance for snapping (as a fraction of grid cell size)
    const float snapThresholdX = 0.1f / horizontalDivisions;
    const float snapThresholdY = 0.1f / verticalDivisions;

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

    return {snappedX, snappedY};
}

const std::vector<std::unique_ptr<EnvelopePoint>> &EnvelopePointManager::getPoints() const {
    return points;
}

void EnvelopePointManager::setPoints(std::vector<std::unique_ptr<EnvelopePoint>> newPoints) {
    points.clear();

    for (auto &point: newPoints) {
        points.push_back(std::make_unique<EnvelopePoint>(
                point->position.x, point->position.y, point->isEditable));
        points.back()->selected = point->selected;
        points.back()->curvature = point->curvature;
    }

    sortPoints();
    notifyPointsChanged();
}

void EnvelopePointManager::sortPoints() {
    std::sort(points.begin(), points.end(),
              [](const auto &a, const auto &b) {
                  return a->position.x < b->position.x;
              });
}

void EnvelopePointManager::notifyPointsChanged() {
    if (onPointsChanged) {
        onPointsChanged();
    }
}

void EnvelopePointManager::setBounds(int width, int height) {
    this->width = width;
    this->height = height;
}
