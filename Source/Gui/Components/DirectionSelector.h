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
        leftButton->setType(Config::DirectionType::LEFT);
        leftButton->setHighlightColor(highlightColour);
        leftButton->onSelectionChanged = [this](Config::DirectionType type)
        { handleSelectionChanged(type); };
        addAndMakeVisible(leftButton.get());

        bidirectionalButton = std::make_unique<DirectionButton>();
        bidirectionalButton->setType(Config::DirectionType::BIDIRECTIONAL);
        bidirectionalButton->setHighlightColor(highlightColour);
        bidirectionalButton->onSelectionChanged = [this](Config::DirectionType type)
        { handleSelectionChanged(type); };
        bidirectionalButton->setSelected(true); // Default selection
        addAndMakeVisible(bidirectionalButton.get());

        rightButton = std::make_unique<DirectionButton>();
        rightButton->setType(Config::DirectionType::RIGHT);
        rightButton->setHighlightColor(highlightColour);
        rightButton->onSelectionChanged = [this](Config::DirectionType type)
        { handleSelectionChanged(type); };
        addAndMakeVisible(rightButton.get());

        randomButton = std::make_unique<DirectionButton>();
        randomButton->setType(Config::DirectionType::RANDOM);
        randomButton->setHighlightColor(highlightColour);
        randomButton->onSelectionChanged = [this](Config::DirectionType type)
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

    void setDirection(Config::DirectionType direction)
    {
        currentDirection = direction;

        leftButton->setSelected(direction == Config::DirectionType::LEFT);
        bidirectionalButton->setSelected(direction
                                         == Config::DirectionType::BIDIRECTIONAL);
        rightButton->setSelected(direction == Config::DirectionType::RIGHT);
        randomButton->setSelected(direction == Config::DirectionType::RANDOM);

        repaint();
    }

    Config::DirectionType getDirection() const { return currentDirection; }

    // Callback for when the selection changes
    std::function<void(Config::DirectionType)> onDirectionChanged;

private:
    void handleSelectionChanged(Config::DirectionType type)
    {
        // Update all buttons
        leftButton->setSelected(type == Config::DirectionType::LEFT);
        bidirectionalButton->setSelected(type == Config::DirectionType::BIDIRECTIONAL);
        rightButton->setSelected(type == Config::DirectionType::RIGHT);
        randomButton->setSelected(type == Config::DirectionType::RANDOM);

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
    Config::DirectionType currentDirection = Config::DirectionType::BIDIRECTIONAL;
};

#endif //JUCECMAKEREPO_DIRECTIONSELECTOR_H
