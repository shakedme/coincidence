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
    DirectionSelector(const juce::String& name, juce::Colour colour)
        : selectorName(name),
        highlightColour(colour)
    {
        // Create the three direction buttons
        leftButton = std::make_unique<DirectionButton>();
        leftButton->setType(Params::DirectionType::LEFT);
        leftButton->setHighlightColor(highlightColour);
        leftButton->onSelectionChanged = [this](Params::DirectionType type) { handleSelectionChanged(type); };
        addAndMakeVisible(leftButton.get());

        bidirectionalButton = std::make_unique<DirectionButton>();
        bidirectionalButton->setType(Params::DirectionType::BIDIRECTIONAL);
        bidirectionalButton->setHighlightColor(highlightColour);
        bidirectionalButton->onSelectionChanged = [this](Params::DirectionType type) { handleSelectionChanged(type); };
        bidirectionalButton->setSelected(true); // Default selection
        addAndMakeVisible(bidirectionalButton.get());

        rightButton = std::make_unique<DirectionButton>();
        rightButton->setType(Params::DirectionType::RIGHT);
        rightButton->setHighlightColor(highlightColour);
        rightButton->onSelectionChanged = [this](Params::DirectionType type) { handleSelectionChanged(type); };
        addAndMakeVisible(rightButton.get());

        // Create the label
        nameLabel = std::make_unique<juce::Label>();
        nameLabel->setText(name, juce::dontSendNotification);
        nameLabel->setFont(juce::Font(11.0f, juce::Font::bold));
        nameLabel->setJustificationType(juce::Justification::centred);
        nameLabel->setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(nameLabel.get());

        // Set initial size
        setSize(70, 40);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Label at the top
        nameLabel->setBounds(bounds.removeFromTop(15));

        // Buttons arranged horizontally
        const int buttonWidth = bounds.getWidth() / 3;

        leftButton->setBounds(bounds.getX(), bounds.getY(), buttonWidth, bounds.getHeight());
        bidirectionalButton->setBounds(bounds.getX() + buttonWidth, bounds.getY(), buttonWidth, bounds.getHeight());
        rightButton->setBounds(bounds.getX() + buttonWidth * 2, bounds.getY(), buttonWidth, bounds.getHeight());
    }

    void setDirection(Params::DirectionType direction)
    {
        currentDirection = direction;

        leftButton->setSelected(direction == Params::DirectionType::LEFT);
        bidirectionalButton->setSelected(direction == Params::DirectionType::BIDIRECTIONAL);
        rightButton->setSelected(direction == Params::DirectionType::RIGHT);

        repaint();
    }

    Params::DirectionType getDirection() const
    {
        return currentDirection;
    }

    // Callback for when the selection changes
    std::function<void(Params::DirectionType)> onDirectionChanged;

private:
    void handleSelectionChanged(Params::DirectionType type)
    {
        // Update all buttons
        leftButton->setSelected(type == Params::DirectionType::LEFT);
        bidirectionalButton->setSelected(type == Params::DirectionType::BIDIRECTIONAL);
        rightButton->setSelected(type == Params::DirectionType::RIGHT);

        currentDirection = type;

        // Notify listeners
        if (onDirectionChanged)
            onDirectionChanged(type);
    }

    std::unique_ptr<DirectionButton> leftButton;
    std::unique_ptr<DirectionButton> bidirectionalButton;
    std::unique_ptr<DirectionButton> rightButton;
    std::unique_ptr<juce::Label> nameLabel;

    juce::String selectorName;
    juce::Colour highlightColour;
    Params::DirectionType currentDirection = Params::DirectionType::BIDIRECTIONAL;
};

#endif //JUCECMAKEREPO_DIRECTIONSELECTOR_H
