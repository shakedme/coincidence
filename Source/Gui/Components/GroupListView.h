#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../../Audio/PluginProcessor.h"

class GroupListView 
    : public juce::Component
    , public juce::Slider::Listener
    , public juce::Timer
{
public:
    explicit GroupListView(PluginProcessor& p)
        : processor(p)
    {
        // Group title
        titleLabel.setText("Sample Groups", juce::dontSendNotification);
        titleLabel.setFont(juce::Font(16.0f, juce::Font::bold));
        titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        titleLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(titleLabel);
        
        // Make the sliders
        for (int i = 0; i < MAX_GROUPS; ++i)
        {
            auto& slider = probabilitySliders[i];
            slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
            slider->setRange(0.0, 1.0, 0.01);
            slider->setValue(1.0);
            slider->setDoubleClickReturnValue(true, 1.0);
            slider->setColour(juce::Slider::rotarySliderFillColourId, getGroupColor(i));
            slider->setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
            slider->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff444444));
            slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
            slider->setNumDecimalPlacesToDisplay(2);
            slider->setEnabled(false); // Initially disabled
            slider->addListener(this);
            addAndMakeVisible(slider.get());
            
            auto& label = groupLabels[i];
            label = std::make_unique<juce::Label>();
            label->setText("Group " + juce::String(i + 1), juce::dontSendNotification);
            label->setFont(juce::Font(14.0f));
            label->setColour(juce::Label::textColourId, juce::Colours::white);
            label->setJustificationType(juce::Justification::centred);
            label->setEnabled(false); // Initially disabled
            addAndMakeVisible(label.get());
        }
        
        // Set initial size
        setSize(300, 200);
        
        // Start a timer to periodically update the UI
        startTimerHz(5);
    }
    
    ~GroupListView() override = default;
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Title at the top
        titleLabel.setBounds(bounds.removeFromTop(24));
        
        // Lay out the groups in a grid (2x2)
        const int numGroups = juce::jmin(static_cast<int>(processor.getSampleManager().getNumGroups()), MAX_GROUPS);
        
        if (numGroups > 0)
        {
            const int groupHeight = bounds.getHeight() / ((numGroups > 2) ? 2 : 1);
            const int groupWidth = bounds.getWidth() / ((numGroups > 2) ? 2 : numGroups);
            
            for (int i = 0; i < numGroups; ++i)
            {
                int row = i / 2;
                int col = i % 2;
                
                juce::Rectangle<int> groupBounds(
                    bounds.getX() + col * groupWidth,
                    bounds.getY() + row * groupHeight,
                    groupWidth,
                    groupHeight
                );
                
                // Label at the top of each group area
                groupLabels[i]->setBounds(groupBounds.removeFromTop(24));
                
                // Slider in the middle of the remaining space
                probabilitySliders[i]->setBounds(groupBounds.reduced(10));
            }
        }
    }
    
    void sliderValueChanged(juce::Slider* slider) override
    {
        // Find which slider changed
        for (int i = 0; i < MAX_GROUPS; ++i)
        {
            if (slider == probabilitySliders[i].get())
            {
                // Update the group probability
                processor.getSampleManager().setGroupProbability(i, static_cast<float>(slider->getValue()));
                break;
            }
        }
    }
    
    void timerCallback() override
    {
        // Check if the number of groups has changed
        const int numGroups = processor.getSampleManager().getNumGroups();
        
        if (numGroups != lastNumGroups)
        {
            lastNumGroups = numGroups;
            
            // Update the UI
            for (int i = 0; i < MAX_GROUPS; ++i)
            {
                bool groupExists = (i < numGroups);
                probabilitySliders[i]->setEnabled(groupExists);
                groupLabels[i]->setEnabled(groupExists);
                
                if (groupExists)
                {
                    // Update group name and probability
                    if (const SampleManager::Group* group = processor.getSampleManager().getGroup(i))
                    {
                        groupLabels[i]->setText(group->name, juce::dontSendNotification);
                        probabilitySliders[i]->setValue(group->probability, juce::dontSendNotification);
                    }
                }
            }
            
            // Resize to update the layout
            resized();
        }
    }
    
private:
    PluginProcessor& processor;
    juce::Label titleLabel;
    
    static constexpr int MAX_GROUPS = 4;
    std::unique_ptr<juce::Slider> probabilitySliders[MAX_GROUPS];
    std::unique_ptr<juce::Label> groupLabels[MAX_GROUPS];
    
    int lastNumGroups = 0;
    
    juce::Colour getGroupColor(int index) const
    {
        // Different colors for each group
        const juce::Colour colors[MAX_GROUPS] = {
            juce::Colour(0xff5c9ce6), // Blue
            juce::Colour(0xff52bf5d), // Green
            juce::Colour(0xffbf5252), // Red
            juce::Colour(0xffbf52d9)  // Purple
        };

        // Make sure index is within range
        const int numColors = sizeof(colors) / sizeof(colors[0]);
        if (index >= 0 && index < numColors)
            return colors[index];

        return juce::Colours::grey;    }
}; 