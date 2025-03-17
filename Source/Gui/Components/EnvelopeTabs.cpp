#include "EnvelopeTabs.h"

EnvelopeTabs::EnvelopeTabs(TabbedButtonBar::Orientation orientation)
    : TabbedComponent(orientation)
{
    // Intercept mouse clicks to prevent the default tab background behavior
    setInterceptsMouseClicks(true, true);
    
    // Set background and outline colors to be transparent
    setColour(TabbedComponent::backgroundColourId, juce::Colours::transparentBlack);
    setColour(TabbedComponent::outlineColourId, juce::Colours::transparentBlack);
    
    // Style the tabs
    auto& tabButtons = getTabbedButtonBar();
    tabButtons.setColour(juce::TabbedButtonBar::tabOutlineColourId, juce::Colours::transparentBlack);
    tabButtons.setColour(juce::TabbedButtonBar::tabTextColourId, juce::Colours::white.withAlpha(0.8f));
    tabButtons.setColour(juce::TabbedButtonBar::frontTextColourId, juce::Colours::white);
    tabButtons.setColour(juce::TabbedButtonBar::frontOutlineColourId, juce::Colours::white.withAlpha(0.5f));
}

void EnvelopeTabs::currentTabChanged(int newCurrentTabIndex, const String &newCurrentTabName)
{
    TabbedComponent::currentTabChanged(newCurrentTabIndex, newCurrentTabName);
    
    // Call the callback if it exists
    if (onTabChanged)
    {
        onTabChanged(newCurrentTabIndex);
    }
} 