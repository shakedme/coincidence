#include "EnvelopeRenderer.h"

EnvelopeRenderer::EnvelopeRenderer(EnvelopePointManager &pointManager,
                                   int hDivisions,
                                   int vDivisions
) :
        horizontalDivisions(hDivisions),
        verticalDivisions(vDivisions),
        pointManager(pointManager) {
}

void EnvelopeRenderer::drawEnvelope(juce::Graphics &g, float transportPosition) {
    drawEnvelopeLine(g);
    drawPositionMarker(g, transportPosition);
    drawPoints(g);
}

void EnvelopeRenderer::drawEnvelopeLine(juce::Graphics &g) {
    const auto &points = pointManager.getPoints();
    if (points.size() < 2) return;

    g.setColour(envelopeColor);

    // Create a path to draw the envelope curve
    juce::Path path;


    juce::Point<float> startPos = pointManager.getPointScreenPosition(0);
    path.startNewSubPath(startPos);

    for (size_t i = 1; i < points.size(); ++i) {
        juce::Point<float> endPos = pointManager.getPointScreenPosition(i);

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

void EnvelopeRenderer::drawPoints(juce::Graphics &g) {
    const auto &points = pointManager.getPoints();

    for (size_t i = 0; i < points.size(); ++i) {
        const auto &point = points[i];
        juce::Point<float> pos = pointManager.getPointScreenPosition(i);

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

void EnvelopeRenderer::drawPositionMarker(juce::Graphics &g, float transportPosition) {
    // Convert transport position (0-1) to screen x-coordinate
    float x = transportPosition * width;

    // Draw vertical line at current position
    g.setColour(positionMarkerColor.withAlpha(0.5f));
    g.drawLine(x, 0, x, static_cast<float>(height - 1), 1.0f);

    // Add a small indicator at the top
    g.setColour(positionMarkerColor);
    g.fillRoundedRectangle(x - 2, 0, 4, 8, 2);
}


void EnvelopeRenderer::drawGrid(juce::Graphics &g) {
    // Draw standard grid
    g.setColour(juce::Colour(0xff444444));

    // Draw vertical lines
    for (int i = 0; i <= horizontalDivisions; ++i) {
        const float x = i * (width / horizontalDivisions);
        g.drawLine(x, 0, x, height - 1, 1.0f);
    }

    // Draw horizontal lines
    for (int i = 0; i <= verticalDivisions; ++i) {
        const float y = i * (height / verticalDivisions);
        g.drawLine(0, y, width, y, 1.0f);
    }

    // Draw center line (value 50%)
    g.setColour(juce::Colour(0xff666666));
    g.drawLine(0, height / 2, width, height / 2, 1.5f);

    g.setColour(juce::Colour(0xff888888));

    // Draw vertical snap lines
    for (int i = 0; i <= horizontalDivisions; ++i) {
        const float x = i * (width / horizontalDivisions);
        g.drawLine(x, 0, x, height - 1, 0.5f);
    }

    // Draw horizontal snap lines
    for (int i = 0; i <= verticalDivisions; ++i) {
        const float y = i * (height / verticalDivisions);
        g.drawLine(0, y, width, y, 0.5f);
    }
}
