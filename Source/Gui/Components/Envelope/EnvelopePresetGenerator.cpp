#include "EnvelopePresetGenerator.h"
#include <cmath>

std::vector<std::unique_ptr<EnvelopePoint>> EnvelopePresetGenerator::createSineShape(int numPoints) {
    std::vector<std::unique_ptr<EnvelopePoint>> points;
    
    // Create a single cycle sine wave with the specified number of points
    for (int i = 0; i < numPoints; ++i) {
        float x = static_cast<float>(i) / (numPoints - 1); // 0 to 1
        // Sine wave oscillates between 0 and 1 (one complete cycle)
        float y = 0.5f + 0.5f * std::sin(x * 2.0f * juce::MathConstants<float>::pi - juce::MathConstants<float>::halfPi);
        points.push_back(std::make_unique<EnvelopePoint>(x, y));
    }
    
    // Ensure first and last point's x positions are exactly 0 and 1
    if (!points.empty()) {
        points.front()->position.x = 0.0f;
        points.back()->position.x = 1.0f;
    }
    
    return points;
}

std::vector<std::unique_ptr<EnvelopePoint>> EnvelopePresetGenerator::createTriangleShape() {
    std::vector<std::unique_ptr<EnvelopePoint>> points;
    
    // Create a triangle wave with 3 points
    points.push_back(std::make_unique<EnvelopePoint>(0.0f, 0.0f));
    points.push_back(std::make_unique<EnvelopePoint>(0.5f, 1.0f));
    points.push_back(std::make_unique<EnvelopePoint>(1.0f, 0.0f));
    
    return points;
}

std::vector<std::unique_ptr<EnvelopePoint>> EnvelopePresetGenerator::createSquareShape() {
    std::vector<std::unique_ptr<EnvelopePoint>> points;
    
    // Create a square wave with 4 points
    points.push_back(std::make_unique<EnvelopePoint>(0.0f, 0.0f));
    points.push_back(std::make_unique<EnvelopePoint>(0.0001f, 1.0f)); // Almost 0 for vertical line
    points.push_back(std::make_unique<EnvelopePoint>(0.5f, 1.0f));
    points.push_back(std::make_unique<EnvelopePoint>(0.5001f, 0.0f)); // Just after 0.5 for vertical line
    points.push_back(std::make_unique<EnvelopePoint>(1.0f, 0.0f));
    
    return points;
}

std::vector<std::unique_ptr<EnvelopePoint>> EnvelopePresetGenerator::createSawtoothShape() {
    std::vector<std::unique_ptr<EnvelopePoint>> points;
    
    // Create a sawtooth wave with 2 points
    points.push_back(std::make_unique<EnvelopePoint>(0.0f, 1.0f));
    points.push_back(std::make_unique<EnvelopePoint>(1.0f, 0.0f));
    
    return points;
}

std::vector<std::unique_ptr<EnvelopePoint>> EnvelopePresetGenerator::createRampUpShape() {
    std::vector<std::unique_ptr<EnvelopePoint>> points;
    
    // Create a ramp up with 2 points
    points.push_back(std::make_unique<EnvelopePoint>(0.0f, 0.0f));
    points.push_back(std::make_unique<EnvelopePoint>(1.0f, 1.0f));
    
    return points;
}

std::vector<std::unique_ptr<EnvelopePoint>> EnvelopePresetGenerator::createRampDownShape() {
    std::vector<std::unique_ptr<EnvelopePoint>> points;
    
    // Create a ramp down with 2 points
    points.push_back(std::make_unique<EnvelopePoint>(0.0f, 1.0f));
    points.push_back(std::make_unique<EnvelopePoint>(1.0f, 0.0f));
    
    return points;
}

std::vector<std::unique_ptr<EnvelopePoint>> EnvelopePresetGenerator::createShape(PresetShape shape) {
    switch (shape) {
        case PresetShape::Sine:
            return createSineShape();
        case PresetShape::Triangle:
            return createTriangleShape();
        case PresetShape::Square:
            return createSquareShape();
        case PresetShape::RampUp:
            return createRampUpShape();
        case PresetShape::RampDown:
            return createRampDownShape();
        default:
            return {};
    }
}