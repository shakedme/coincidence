#pragma once

#include <vector>
#include <memory>
#include "../../../Shared/Envelope/EnvelopePoint.h"
#include <juce_core/juce_core.h>

/**
 * Utility class to generate different preset shapes for envelope
 */
class EnvelopePresetGenerator {
public:
    // Preset shape enum
    enum class PresetShape {
        Sine,
        Triangle,
        Square,
        RampUp,
        RampDown,
        Custom
    };

    // Creates points for a sine wave shape
    static std::vector<std::unique_ptr<EnvelopePoint>> createSineShape(int numPoints = 100);

    // Creates points for a triangle wave shape
    static std::vector<std::unique_ptr<EnvelopePoint>> createTriangleShape();

    // Creates points for a square wave shape
    static std::vector<std::unique_ptr<EnvelopePoint>> createSquareShape();

    // Creates points for a sawtooth wave shape
    static std::vector<std::unique_ptr<EnvelopePoint>> createSawtoothShape();

    // Creates points for a ramp up shape
    static std::vector<std::unique_ptr<EnvelopePoint>> createRampUpShape();

    // Creates points for a ramp down shape
    static std::vector<std::unique_ptr<EnvelopePoint>> createRampDownShape();

    static std::vector<std::unique_ptr<EnvelopePoint>> createShape(PresetShape shape);
}; 