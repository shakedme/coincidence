#include "EnvelopeRenderer.h"

EnvelopeRenderer::EnvelopeRenderer(EnvelopePointManager &pointManager)
        : pointManager(pointManager) {
}

void EnvelopeRenderer::drawEnvelope(juce::Graphics &g, int width, int height, float transportPosition) {
    // Draw the envelope line
    drawEnvelopeLine(g, width, height);

    // Draw the position marker
    drawPositionMarker(g, width, height, transportPosition);

    // Draw the points
    drawPoints(g, width, height);
}

void EnvelopeRenderer::drawEnvelopeLine(juce::Graphics &g, int width, int height) {
    const auto &points = pointManager.getPoints();
    if (points.size() < 2) return;

    g.setColour(envelopeColor);

    // Create a path to draw the envelope curve
    juce::Path path;

    juce::Point<float> startPos = pointManager.getPointScreenPosition(0, width, height);
    path.startNewSubPath(startPos);

    for (size_t i = 1; i < points.size(); ++i) {
        juce::Point<float> endPos = pointManager.getPointScreenPosition(i, width, height);

        if (points[i]->curvature != 0.0f) {
            // Calculate control points for a quadratic bezier curve
            const float curvature = points[i]->curvature;

            // For visual display, use a large multiplier, but invert the sign to match parameter behavior
            // Negative curvature = bend down, positive curvature = bend up
            const float curveAmount = -100.0f * curvature; // Invert direction for visual consistency

            // Calculate control point position
            juce::Point<float> midPoint = startPos + (endPos - startPos) * 0.5f;
            juce::Point<float> perpendicular(-((endPos.y - startPos.y)), (endPos.x - startPos.x));

            // Manually normalize the perpendicular vector
            float length = std::sqrt(perpendicular.x * perpendicular.x + perpendicular.y * perpendicular.y);
            if (length > 0.0f) {
                perpendicular.x = perpendicular.x / length * curveAmount;
                perpendicular.y = perpendicular.y / length * curveAmount;
            }

            juce::Point<float> controlPoint = midPoint + perpendicular;

            // Draw the curved segment
            path.quadraticTo(controlPoint, endPos);
        } else {
            // Draw straight line
            path.lineTo(endPos);
        }

        startPos = endPos;
    }

    g.strokePath(path, juce::PathStrokeType(2.0f));
}

void EnvelopeRenderer::drawPoints(juce::Graphics &g, int width, int height) {
    const auto &points = pointManager.getPoints();

    for (size_t i = 0; i < points.size(); ++i) {
        const auto &point = points[i];
        juce::Point<float> pos = pointManager.getPointScreenPosition(i, width, height);

        // Draw point
        if (point->selected) {
            g.setColour(selectedPointColor);
            g.fillEllipse(pos.x - pointRadius, pos.y - pointRadius,
                          pointRadius * 2, pointRadius * 2);

            g.setColour(pointColor);
            g.drawEllipse(pos.x - pointRadius, pos.y - pointRadius,
                          pointRadius * 2, pointRadius * 2, 2.0f);
        } else {
            g.setColour(pointColor);
            g.fillEllipse(pos.x - pointRadius, pos.y - pointRadius,
                          pointRadius * 2, pointRadius * 2);
        }

        // If point has curvature, indicate it with a small mark
        if (point->curvature != 0.0f) {
            g.setColour(juce::Colours::yellow);
            g.fillEllipse(pos.x - 2.0f, pos.y - 2.0f, 4.0f, 4.0f);
        }
    }
}

void EnvelopeRenderer::drawSelectionArea(juce::Graphics &g, const juce::Rectangle<float> &area) {
    // Semi-transparent fill
    g.setColour(selectionFillColor);
    g.fillRect(area);

    // Border
    g.setColour(selectionOutlineColor);
    g.drawRect(area, 1.0f);
}

void EnvelopeRenderer::drawPositionMarker(juce::Graphics &g, int width, int height, float transportPosition) {
    // Convert transport position (0-1) to screen x-coordinate
    float x = transportPosition * width;

    // Draw vertical line at current position
    g.setColour(positionMarkerColor.withAlpha(0.5f));
    g.drawLine(x, 0, x, static_cast<float>(height), 1.0f);

    // Add a small indicator at the top
    g.setColour(positionMarkerColor);
    g.fillRoundedRectangle(x - 2, 0, 4, 8, 2);
}