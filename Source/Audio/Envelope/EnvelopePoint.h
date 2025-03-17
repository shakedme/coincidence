#pragma once

#include <juce_core/juce_core.h>

// Represents a control point on an envelope
class EnvelopePoint {
public:
    EnvelopePoint(float x, float y, bool _isEditable = true) 
        : position(x, y), isEditable(_isEditable) {
    }

    juce::Point<float> position;
    bool selected = false;
    bool isEditable = true;
    float curvature = 0.0f; // 0.0 = straight line, -1.0 to 1.0 = curved
}; 