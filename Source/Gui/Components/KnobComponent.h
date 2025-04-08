//
// Created by Shaked Melman on 01/03/2025.
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>


class KnobComponent : public juce::Slider,
                      public juce::DragAndDropTarget {
public:
    KnobComponent(const juce::String &tooltip);

    ~KnobComponent() override = default;

    // Override paint to add drag highlighting
    void paint(juce::Graphics &g) override;

    // DragAndDropTarget implementation
    bool isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override;

    void itemDragEnter(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override;

    void itemDragMove(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override;

    void itemDragExit(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override;

    void itemDropped(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override;

private:
    bool dragHighlight = false;
}; 