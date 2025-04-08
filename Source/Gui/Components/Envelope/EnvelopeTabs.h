#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>


class EnvelopeTabs :
        public juce::TabbedComponent,
        public juce::DragAndDropTarget {
public:
    explicit EnvelopeTabs(juce::TabbedButtonBar::Orientation orientation)
            : juce::TabbedComponent(orientation) {
        setInterceptsMouseClicks(false, true);
        setColour(juce::TabbedComponent::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::TabbedComponent::outlineColourId, juce::Colours::transparentBlack);
    }

    std::function<void(int tabIndex)> onTabChanged;

    void currentTabChanged(int newCurrentTabIndex, const juce::String & /*newCurrentTabName*/) override {
        if (onTabChanged)
            onTabChanged(newCurrentTabIndex);
    }

    void addTab(const juce::String &tabName, const juce::Colour &backgroundColour, juce::Component *contentComponent,
                bool insertBeforeCurrentTab) {
        juce::TabbedComponent::addTab(tabName, backgroundColour, contentComponent, insertBeforeCurrentTab);

        auto *button = getTabbedButtonBar().getTabButton(getNumTabs() - 1);
        if (button != nullptr) {
            button->setMouseCursor(juce::MouseCursor::DraggingHandCursor);
            button->addMouseListener(this, false);
        }
    }

    bool isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override {
        juce::ignoreUnused(dragSourceDetails);
        return false;
    }

    void itemDragEnter(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override {
        juce::ignoreUnused(dragSourceDetails);
    }

    void itemDragMove(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override {
        juce::ignoreUnused(dragSourceDetails);
    }

    void itemDragExit(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override {
        juce::ignoreUnused(dragSourceDetails);
    }

    void itemDropped(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override {
        juce::ignoreUnused(dragSourceDetails);
    }

    void mouseDrag(const juce::MouseEvent &e) override {
        auto *button = dynamic_cast<juce::TabBarButton *>(e.eventComponent);
        if (button != nullptr) {
            int tabIndex = button->getIndex();
            if (tabIndex >= 0) {
                auto *dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this);
                if (dragContainer != nullptr) {
                    juce::var lfoIndex = tabIndex;
                    juce::Image dragImage(juce::Image::ARGB, button->getWidth(), button->getHeight(), true);
                    juce::Graphics g(dragImage);
                    button->paintEntireComponent(g, false);
                    dragContainer->startDragging(lfoIndex, button, dragImage, true);
                }
            }
        }
    }
}; 