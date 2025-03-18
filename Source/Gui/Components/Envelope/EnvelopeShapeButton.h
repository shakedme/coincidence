//
// Created by Shaked Melman on 18/03/2025.
//

#ifndef COINCIDENCE_ENVELOPESHAPEBUTTON_H
#define COINCIDENCE_ENVELOPESHAPEBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "EnvelopePresetGenerator.h"

class EnvelopeShapeButton : public juce::Button {
public:
    explicit EnvelopeShapeButton(const juce::String &name, EnvelopePresetGenerator::PresetShape shapeType)
            : juce::Button(name), shape(shapeType) {}

    void paintButton(juce::Graphics &g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
        // Background
        g.fillAll(juce::Colours::black.withAlpha(0.3f));

        // Border
        if (shouldDrawButtonAsHighlighted) {
            g.setColour(juce::Colours::white);
            g.drawRect(getLocalBounds().toFloat(), 1.5f);
        } else {
            g.setColour(juce::Colours::grey);
            g.drawRect(getLocalBounds().toFloat(), 1.0f);
        }

        // Draw the shape
        auto bounds = getLocalBounds().toFloat().reduced(4.0f);
        drawShape(g, bounds);
    }

private:
    void drawShape(juce::Graphics &g, const juce::Rectangle<float> &bounds) {
        g.setColour(juce::Colour(0xff52bfd9));

        const float width = bounds.getWidth();
        const float height = bounds.getHeight();
        const float x = bounds.getX();
        const float y = bounds.getY();

        juce::Path path;

        switch (shape) {
            case EnvelopePresetGenerator::PresetShape::Sine: {
                path.startNewSubPath(x, y + height / 2);

                for (float i = 0; i <= width; i += 1.0f) {
                    float t = i / width;
                    float value = 0.5f * (1.0f - std::sin(juce::MathConstants<float>::twoPi * t));
                    path.lineTo(x + i, y + value * height);
                }
                break;
            }

            case EnvelopePresetGenerator::PresetShape::Triangle: {
                path.startNewSubPath(x, y + height);
                path.lineTo(x + width * 0.5f, y);
                path.lineTo(x + width, y + height);
                break;
            }

            case EnvelopePresetGenerator::PresetShape::Square: {
                path.startNewSubPath(x, y + height);
                path.lineTo(x, y);
                path.lineTo(x + width * 0.5f, y);
                path.lineTo(x + width * 0.5f, y + height);
                path.lineTo(x + width, y + height);
                break;
            }

            case EnvelopePresetGenerator::PresetShape::RampUp: {
                path.startNewSubPath(x, y + height);
                path.lineTo(x + width, y);
                break;
            }

            case EnvelopePresetGenerator::PresetShape::RampDown: {
                path.startNewSubPath(x, y);
                path.lineTo(x + width, y + height);
                break;
            }

            default:
                break;
        }

        g.strokePath(path, juce::PathStrokeType(1.5f));
    }

    EnvelopePresetGenerator::PresetShape shape;
};

#endif //COINCIDENCE_ENVELOPESHAPEBUTTON_H
