#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

//==============================================================================
class HeaderComponent : public juce::Component
{
public:
    enum class Tab
    {
        Main,
        Env
    };

    HeaderComponent()
    {
        // Add title label
        titleLabel = std::make_unique<juce::Label>("titleLabel", "Coincidence");
        titleLabel->setFont(juce::Font(24.0f, juce::Font::bold));
        titleLabel->setJustificationType(juce::Justification::centred);
        titleLabel->setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(titleLabel.get());

        // Add tabs
        mainTabButton = std::make_unique<juce::TextButton>("Main");
        mainTabButton->setRadioGroupId(1);
        mainTabButton->setClickingTogglesState(true);
        mainTabButton->setToggleState(true, juce::dontSendNotification);
        mainTabButton->onClick = [this] { setActiveTab(Tab::Main); };
        addAndMakeVisible(mainTabButton.get());

        envTabButton = std::make_unique<juce::TextButton>("Env");
        envTabButton->setRadioGroupId(1);
        envTabButton->setClickingTogglesState(true);
        envTabButton->onClick = [this] { setActiveTab(Tab::Env); };
        addAndMakeVisible(envTabButton.get());
    }

    ~HeaderComponent() override = default;

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff333333));
        
        // Add highlight
        g.setColour(juce::Colour(0x20ffffff));
        g.fillRect(0, 3, getWidth(), 2);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        
        // Position tab buttons on the left
        const int tabWidth = 80;
        const int tabHeight = 25;
        const int tabY = (getHeight() - tabHeight) / 2;
        
        mainTabButton->setBounds(10, tabY, tabWidth, tabHeight);
        envTabButton->setBounds(10 + tabWidth + 5, tabY, tabWidth, tabHeight);
        
        // Center the title
        titleLabel->setBounds(area.reduced(tabWidth * 2 + 30, 0));
    }

    void setActiveTab(Tab tab)
    {
        activeTab = tab;
        
        mainTabButton->setToggleState(tab == Tab::Main, juce::dontSendNotification);
        envTabButton->setToggleState(tab == Tab::Env, juce::dontSendNotification);
        
        if (onTabChanged != nullptr)
            onTabChanged(tab);
    }
    
    Tab getActiveTab() const { return activeTab; }
    
    std::function<void(Tab)> onTabChanged;

private:
    std::unique_ptr<juce::Label> titleLabel;
    std::unique_ptr<juce::TextButton> mainTabButton;
    std::unique_ptr<juce::TextButton> envTabButton;
    Tab activeTab = Tab::Main;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderComponent)
}; 