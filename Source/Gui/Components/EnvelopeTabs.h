#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class EnvelopeTabs : public juce::TabbedComponent {
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
}; 