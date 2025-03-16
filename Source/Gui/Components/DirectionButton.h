//
// Created by Shaked Melman on 02/03/2025.
//

#ifndef JUCECMAKEREPO_DIRECTIONBUTTON_H
#define JUCECMAKEREPO_DIRECTIONBUTTON_H

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Shared/Models.h"

class DirectionButton : public juce::Component
{
public:
    DirectionButton() { setSize(40, 20); }

    void paint(juce::Graphics& g) override
    {
        // Background
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);

        // Determine if the mouse is currently over this button
        bool isOver = isMouseOver();
        bool isDown = isMouseButtonDown();

        // Color based on state
        juce::Colour arrowColor;
        if (isSelected)
            arrowColor = highlightColor;
        else if (isDown)
            arrowColor = juce::Colours::white.withAlpha(0.8f);
        else if (isOver)
            arrowColor = juce::Colours::white.withAlpha(0.6f);
        else
            arrowColor = juce::Colours::white.withAlpha(0.4f);

        g.setColour(arrowColor);

        // Draw the appropriate direction arrows
        switch (type)
        {
            case Models::DirectionType::LEFT:
                drawLeftArrow(g, bounds, arrowColor);
                break;

            case Models::DirectionType::BIDIRECTIONAL:
                drawBidirectionalArrows(g, bounds, arrowColor);
                break;

            case Models::DirectionType::RIGHT:
                drawRightArrow(g, bounds, arrowColor);
                break;

            case Models::DirectionType::RANDOM:
                drawQuestionMark(g, bounds, arrowColor);
                break;
        }
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        setSelected(true);
        if (onSelectionChanged)
            onSelectionChanged(type);

        repaint();
    }

    void mouseEnter(const juce::MouseEvent&) override { repaint(); }

    void mouseExit(const juce::MouseEvent&) override { repaint(); }

    void setType(Models::DirectionType newType)
    {
        type = newType;
        repaint();
    }

    Models::DirectionType getType() const { return type; }

    void setSelected(bool shouldBeSelected)
    {
        isSelected = shouldBeSelected;
        repaint();
    }

    bool getSelected() const { return isSelected; }

    void setHighlightColor(juce::Colour color)
    {
        highlightColor = color;
        repaint();
    }

    std::function<void(Models::DirectionType)> onSelectionChanged;

private:
    void drawLeftArrow(juce::Graphics& g,
                       juce::Rectangle<float> bounds,
                       juce::Colour color)
    {
        float arrowSize = bounds.getHeight() * 0.5f;
        float centerY = bounds.getCentreY();

        juce::Path arrow;
        arrow.startNewSubPath(bounds.getRight() - 4, centerY - arrowSize / 2);
        arrow.lineTo(bounds.getX() + 4, centerY);
        arrow.lineTo(bounds.getRight() - 4, centerY + arrowSize / 2);

        g.setColour(color);
        g.strokePath(arrow, juce::PathStrokeType(2.0f));
    }

    void drawRightArrow(juce::Graphics& g,
                        juce::Rectangle<float> bounds,
                        juce::Colour color)
    {
        float arrowSize = bounds.getHeight() * 0.5f;
        float centerY = bounds.getCentreY();

        juce::Path arrow;
        arrow.startNewSubPath(bounds.getX() + 4, centerY - arrowSize / 2);
        arrow.lineTo(bounds.getRight() - 4, centerY);
        arrow.lineTo(bounds.getX() + 4, centerY + arrowSize / 2);

        g.setColour(color);
        g.strokePath(arrow, juce::PathStrokeType(2.0f));
    }

    void drawQuestionMark(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color)
    {
        float width = bounds.getWidth();
        float height = bounds.getHeight();
        float centerX = bounds.getCentreX();
        float centerY = bounds.getCentreY();
        float size = juce::jmin(width, height) * 0.7f;

        g.setColour(color);

        juce::Path questionMark;

        // Main curve of question mark
        questionMark.startNewSubPath(centerX, centerY - size * 0.1f);
        questionMark.lineTo(centerX, centerY - size * 0.15f);
        questionMark.quadraticTo(centerX, centerY - size * 0.35f,
                                 centerX + size * 0.2f, centerY - size * 0.35f);
        questionMark.quadraticTo(centerX + size * 0.4f, centerY - size * 0.35f,
                                 centerX + size * 0.4f, centerY - size * 0.15f);
        questionMark.quadraticTo(centerX + size * 0.4f, centerY + 0.05f * size,
                                 centerX + size * 0.2f, centerY + 0.05f * size);
        questionMark.lineTo(centerX, centerY + 0.05f * size);

        // Question mark dot - positioned further down
        float dotSize = size * 0.1f;
        questionMark.addEllipse(centerX - dotSize/2,
                                centerY + size * 0.35f,
                                dotSize, dotSize);

        g.strokePath(questionMark, juce::PathStrokeType(2.0f));
    }

    void drawBidirectionalArrows(juce::Graphics& g,
                                 juce::Rectangle<float> bounds,
                                 juce::Colour)
    {
        float arrowSize = bounds.getHeight() * 0.4f;
        float centerY = bounds.getCentreY();
        float centerX = bounds.getCentreX();

        // Left arrow
        juce::Path leftArrow;
        leftArrow.startNewSubPath(centerX - 2, centerY - arrowSize / 2);
        leftArrow.lineTo(bounds.getX() + 4, centerY);
        leftArrow.lineTo(centerX - 2, centerY + arrowSize / 2);

        // Right arrow
        juce::Path rightArrow;
        rightArrow.startNewSubPath(centerX + 2, centerY - arrowSize / 2);
        rightArrow.lineTo(bounds.getRight() - 4, centerY);
        rightArrow.lineTo(centerX + 2, centerY + arrowSize / 2);

        g.strokePath(leftArrow, juce::PathStrokeType(2.0f));
        g.strokePath(rightArrow, juce::PathStrokeType(2.0f));
    }

    Models::DirectionType type = Models::DirectionType::BIDIRECTIONAL;
    bool isSelected = false;
    juce::Colour highlightColor = juce::Colours::lime;
};
#endif //JUCECMAKEREPO_DIRECTIONBUTTON_H
