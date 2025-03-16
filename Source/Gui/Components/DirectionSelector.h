//
// Created by Shaked Melman on 02/03/2025.
//

#ifndef JUCECMAKEREPO_DIRECTIONSELECTOR_H
#define JUCECMAKEREPO_DIRECTIONSELECTOR_H

#pragma once

#include "DirectionButton.h"

class DirectionSelector : public juce::Component
{
public:
    DirectionSelector(juce::Colour colour)
        : highlightColour(colour)
    {
        // Create the three direction buttons
        leftButton = std::make_unique<DirectionButton>();
        leftButton->setType(Models::DirectionType::LEFT);
        leftButton->setHighlightColor(highlightColour);
        leftButton->onSelectionChanged = [this](Models::DirectionType type)
        { handleSelectionChanged(type); };
        addAndMakeVisible(leftButton.get());

        bidirectionalButton = std::make_unique<DirectionButton>();
        bidirectionalButton->setType(Models::DirectionType::BIDIRECTIONAL);
        bidirectionalButton->setHighlightColor(highlightColour);
        bidirectionalButton->onSelectionChanged = [this](Models::DirectionType type)
        { handleSelectionChanged(type); };
        bidirectionalButton->setSelected(true); // Default selection
        addAndMakeVisible(bidirectionalButton.get());

        rightButton = std::make_unique<DirectionButton>();
        rightButton->setType(Models::DirectionType::RIGHT);
        rightButton->setHighlightColor(highlightColour);
        rightButton->onSelectionChanged = [this](Models::DirectionType type)
        { handleSelectionChanged(type); };
        addAndMakeVisible(rightButton.get());

        randomButton = std::make_unique<DirectionButton>();
        randomButton->setType(Models::DirectionType::RANDOM);
        randomButton->setHighlightColor(highlightColour);
        randomButton->onSelectionChanged = [this](Models::DirectionType type)
        { handleSelectionChanged(type); };
        addAndMakeVisible(randomButton.get());
        // Set initial size
        setSize(70, 40);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Buttons arranged horizontally
        const int buttonWidth = bounds.getWidth() / 4;

        leftButton->setBounds(
            bounds.getX(), bounds.getY(), buttonWidth, bounds.getHeight());
        bidirectionalButton->setBounds(
            bounds.getX() + buttonWidth, bounds.getY(), buttonWidth, bounds.getHeight());
        rightButton->setBounds(bounds.getX() + buttonWidth * 2,
                               bounds.getY(),
                               buttonWidth,
                               bounds.getHeight());
        randomButton->setBounds(bounds.getX() + buttonWidth * 3,
                                bounds.getY(),
                                buttonWidth,
                                bounds.getHeight());
    }

    void setDirection(Models::DirectionType direction)
    {
        currentDirection = direction;

        leftButton->setSelected(direction == Models::DirectionType::LEFT);
        bidirectionalButton->setSelected(direction
                                         == Models::DirectionType::BIDIRECTIONAL);
        rightButton->setSelected(direction == Models::DirectionType::RIGHT);
        randomButton->setSelected(direction == Models::DirectionType::RANDOM);

        repaint();
    }

    Models::DirectionType getDirection() const { return currentDirection; }

    // Callback for when the selection changes
    std::function<void(Models::DirectionType)> onDirectionChanged;

private:
    void handleSelectionChanged(Models::DirectionType type)
    {
        // Update all buttons
        leftButton->setSelected(type == Models::DirectionType::LEFT);
        bidirectionalButton->setSelected(type == Models::DirectionType::BIDIRECTIONAL);
        rightButton->setSelected(type == Models::DirectionType::RIGHT);
        randomButton->setSelected(type == Models::DirectionType::RANDOM);

        currentDirection = type;

        // Notify listeners
        if (onDirectionChanged)
            onDirectionChanged(type);
    }

    std::unique_ptr<DirectionButton> leftButton;
    std::unique_ptr<DirectionButton> bidirectionalButton;
    std::unique_ptr<DirectionButton> rightButton;
    std::unique_ptr<DirectionButton> randomButton;

    juce::String selectorName;
    juce::Colour highlightColour;
    Models::DirectionType currentDirection = Models::DirectionType::BIDIRECTIONAL;
};

#endif //JUCECMAKEREPO_DIRECTIONSELECTOR_H
