//
// Created by Shaked Melman on 01/03/2025.
//

#include "KnobComponent.h"
#include "../Sections/BaseSection.h"
#include "../Sections/EnvelopeSection.h"
#include "../PluginEditor.h"
#include "../Util.h"
#include <juce_gui_basics/juce_gui_basics.h>

KnobComponent::KnobComponent(ModulationMatrix &modMatrix, const juce::String &tooltip)
        : modMatrix(modMatrix),
          juce::Slider(juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow) {
    setTooltip(tooltip);
    setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 12);
    setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setNumDecimalPlacesToDisplay(0);
    startTimerHz(30);
}

void KnobComponent::paint(juce::Graphics &g) {
    juce::Slider::paint(g);

    if (dragHighlight) {
        auto bounds = getLocalBounds().toFloat();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();

        juce::Colour highlightColor;
        juce::Component *parent = getParentComponent();

        while (parent != nullptr) {
            if (auto *section = dynamic_cast<BaseSectionComponent *>(parent)) {
                highlightColor = section->getSectionColour();
                break;
            }
            parent = parent->getParentComponent();
        }

        g.setColour(highlightColor.withAlpha(0.6f));
        g.drawEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 2.0f);

        for (int i = 0; i < 3; ++i) {
            float alpha = 0.3f - i * 0.1f;
            float expansion = i * 2.0f;
            g.setColour(highlightColor.withAlpha(alpha));
            g.drawEllipse(centreX - radius - expansion,
                          centreY - radius - expansion,
                          (radius + expansion) * 2.0f,
                          (radius + expansion) * 2.0f,
                          1.0f);
        }
    }

    if (isModulated) {
        auto bounds = getLocalBounds().toFloat();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f - 5.0f;
        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();

        // Draw modulation arc
        juce::Path arcPath;
        float startAngle = juce::MathConstants<float>::pi * 1.2f;
        float endAngle = juce::MathConstants<float>::pi * 2.8f;
        float modAngle = startAngle + (endAngle - startAngle) * (getValue() + modulationValue);

        arcPath.addArc(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f,
                       startAngle, endAngle, true);

        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.strokePath(arcPath, juce::PathStrokeType(2.0f));

        // Draw modulation indicator dot
        float dotX = centreX + std::cos(modAngle) * radius;
        float dotY = centreY + std::sin(modAngle) * radius;
        g.setColour(juce::Colours::white);
        g.fillEllipse(dotX - 3.0f, dotY - 3.0f, 6.0f, 6.0f);
    }
}

bool KnobComponent::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) {
    return dragSourceDetails.description.isInt();
}

void KnobComponent::itemDragEnter(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) {
    dragHighlight = true;
    repaint();
}

void KnobComponent::itemDragMove(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) {
    // Handle drag move - no specific action needed
}

void KnobComponent::itemDragExit(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) {
    dragHighlight = false;
    repaint();
}

void KnobComponent::itemDropped(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) {
    dragHighlight = false;

    if (dragSourceDetails.description.isInt()) {
        int lfoIndex = dragSourceDetails.description;
        auto *dragSource = dynamic_cast<juce::Component *>(dragSourceDetails.sourceComponent.get());
        if (dragSource != nullptr) {
            auto *editor = findParentComponentOfClass<PluginEditor>();
            auto *envelopeSection = Util::getChildComponentOfClass<EnvelopeSection>(editor);
            auto lfoComponent = envelopeSection->getLFOComponent(lfoIndex);
            modMatrix.addConnection(lfoComponent, getName());
            isModulated = true;
        }
    }

    repaint();
}

void KnobComponent::timerCallback() {
    if (isModulated) {
        auto [baseValue, modValue] = modMatrix.getParamAndModulationValue(getName());
        if (modValue != modulationValue) {
            modulationValue = modValue;
            isModulated = modValue != 0.0f;
        }
        repaint();
    }
}