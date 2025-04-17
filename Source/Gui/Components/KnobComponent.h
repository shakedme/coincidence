//
// Created by Shaked Melman on 01/03/2025.
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Shared/ModulationMatrix.h"


class KnobComponent : public juce::Slider,
                      public juce::Timer,
                      public juce::DragAndDropTarget {
public:
    KnobComponent(ModulationMatrix &modMatrix, const juce::String &tooltip);

    ~KnobComponent() override = default;

    // Override paint to add drag highlighting
    void paint(juce::Graphics &g) override;

    // DragAndDropTarget implementation
    bool isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override;

    void itemDragEnter(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override;

    void itemDragMove(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override;

    void itemDragExit(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override;

    void itemDropped(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override;

    void timerCallback() override;

private:
    ModulationMatrix &modMatrix;
    bool dragHighlight = false;
    bool isModulated = false;
    float modulationValue = 0.0f;
}; 