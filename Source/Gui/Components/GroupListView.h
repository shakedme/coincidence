#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../../Audio/PluginProcessor.h"
#include "Icon.h"

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
        
        // Set title visibility - we'll control this externally
        titleLabel.setVisible(false);
        
        // Make the sliders and controls for each group
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
            slider->setNumDecimalPlacesToDisplay(0);
            slider->setEnabled(false); // Initially disabled
            slider->addListener(this);
            slider->setTooltip("Group probability");
            addAndMakeVisible(slider.get());
            
            auto& label = groupLabels[i];
            label = std::make_unique<juce::Label>();
            label->setText("Group " + juce::String(i + 1), juce::dontSendNotification);
            label->setFont(juce::Font(14.0f));
            label->setColour(juce::Label::textColourId, juce::Colours::white);
            label->setJustificationType(juce::Justification::centred);
            label->setEnabled(false); // Initially disabled
            addAndMakeVisible(label.get());
            
            // Create rate icons
            setupRateIcon(rateIcons1_2[i], "1/2", Params::RATE_1_2, i);
            setupRateIcon(rateIcons1_4[i], "1/4", Params::RATE_1_4, i);
            setupRateIcon(rateIcons1_8[i], "1/8", Params::RATE_1_8, i);
            setupRateIcon(rateIcons1_16[i], "1/16", Params::RATE_1_16, i);
            
            // Create effect icons
            setupEffectIcon(reverbIcons[i], "R", i, EffectType::Reverb);
            setupEffectIcon(stutterIcons[i], "S", i, EffectType::Stutter);
            setupEffectIcon(delayIcons[i], "D", i, EffectType::Delay);
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
        
        // Title at the top (only takes space if visible)
        if (titleLabel.isVisible())
        {
            titleLabel.setBounds(bounds.removeFromTop(24));
        }
        
        // Lay out the groups in a single row
        const int numGroups = juce::jmin(static_cast<int>(processor.getSampleManager().getNumGroups()), MAX_GROUPS);
        
        if (numGroups > 0)
        {
            // Calculate group width based on all groups in a single row
            const int groupWidth = bounds.getWidth() / numGroups;
            
            for (int i = 0; i < numGroups; ++i)
            {
                juce::Rectangle<int> groupBounds(
                    bounds.getX() + i * groupWidth,
                    bounds.getY(),
                    groupWidth,
                    bounds.getHeight()
                );
                
                // Label at the top of each group area
                groupLabels[i]->setBounds(groupBounds.removeFromTop(24));
                
                // Calculate space for all controls
                int sliderSize = 40;
                int rateIconWidth = 27;
                int iconSize = 20;
                int padding = 5;
                
                // Position the slider in the center
                juce::Rectangle<int> sliderArea = groupBounds.withSizeKeepingCentre(sliderSize, sliderSize);
                probabilitySliders[i]->setBounds(sliderArea);
                
                // Position rate icons in a row below the slider
                int rateY = sliderArea.getBottom() + padding;
                int rateX = groupBounds.getCentreX() - ((rateIconWidth * 4 + padding * 3) / 2);
                
                rateIcons1_2[i]->setBounds(rateX, rateY, rateIconWidth, iconSize);
                rateX += rateIconWidth + padding;
                rateIcons1_4[i]->setBounds(rateX, rateY, rateIconWidth, iconSize);
                rateX += rateIconWidth + padding;
                rateIcons1_8[i]->setBounds(rateX, rateY, rateIconWidth, iconSize);
                rateX += rateIconWidth + padding;
                rateIcons1_16[i]->setBounds(rateX, rateY, rateIconWidth, iconSize);
                
                // Position effect icons in a row above the slider
                int effectY = sliderArea.getY() - iconSize - padding;
                int effectX = groupBounds.getCentreX() - ((iconSize * 3 + padding * 2) / 2);
                
                reverbIcons[i]->setBounds(effectX, effectY, iconSize, iconSize);
                effectX += iconSize + padding;
                stutterIcons[i]->setBounds(effectX, effectY, iconSize, iconSize);
                effectX += iconSize + padding;
                delayIcons[i]->setBounds(effectX, effectY, iconSize, iconSize);
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
                
                // Enable/disable all rate and effect icons
                rateIcons1_2[i]->setVisible(groupExists);
                rateIcons1_4[i]->setVisible(groupExists);
                rateIcons1_8[i]->setVisible(groupExists);
                rateIcons1_16[i]->setVisible(groupExists);
                
                reverbIcons[i]->setVisible(groupExists);
                stutterIcons[i]->setVisible(groupExists);
                delayIcons[i]->setVisible(groupExists);
                
                if (groupExists)
                {
                    // Update group name and probability
                    if (const SampleManager::Group* group = processor.getSampleManager().getGroup(i))
                    {
                        groupLabels[i]->setText(group->name, juce::dontSendNotification);
                        probabilitySliders[i]->setValue(group->probability, juce::dontSendNotification);
                        
                        // Update rate icon states (you need to implement these methods in SampleManager)
                        updateRateIconState(rateIcons1_2[i].get(), i, Params::RATE_1_2);
                        updateRateIconState(rateIcons1_4[i].get(), i, Params::RATE_1_4);
                        updateRateIconState(rateIcons1_8[i].get(), i, Params::RATE_1_8);
                        updateRateIconState(rateIcons1_16[i].get(), i, Params::RATE_1_16);
                        
                        // Update effect icon states (you need to implement these methods in SampleManager)
                        updateEffectIconState(reverbIcons[i].get(), i, EffectType::Reverb);
                        updateEffectIconState(stutterIcons[i].get(), i, EffectType::Stutter);
                        updateEffectIconState(delayIcons[i].get(), i, EffectType::Delay);
                    }
                }
            }
            
            // Resize to update the layout
            resized();
        }
    }
    
    // Add method to control title visibility
    void setTitleVisible(bool shouldBeVisible)
    {
        titleLabel.setVisible(shouldBeVisible);
    }
    
private:
    enum class EffectType { Reverb, Stutter, Delay };

    PluginProcessor& processor;
    juce::Label titleLabel;
    
    static constexpr int MAX_GROUPS = 4;
    std::unique_ptr<juce::Slider> probabilitySliders[MAX_GROUPS];
    std::unique_ptr<juce::Label> groupLabels[MAX_GROUPS];
    
    // Rate icons (1/2, 1/4, 1/8, 1/16) for each group
    std::unique_ptr<TextIcon> rateIcons1_2[MAX_GROUPS];
    std::unique_ptr<TextIcon> rateIcons1_4[MAX_GROUPS];
    std::unique_ptr<TextIcon> rateIcons1_8[MAX_GROUPS];
    std::unique_ptr<TextIcon> rateIcons1_16[MAX_GROUPS];
    
    // Effect icons (reverb, stutter, delay) for each group
    std::unique_ptr<TextIcon> reverbIcons[MAX_GROUPS];
    std::unique_ptr<TextIcon> stutterIcons[MAX_GROUPS];
    std::unique_ptr<TextIcon> delayIcons[MAX_GROUPS];
    
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

        return juce::Colours::grey;
    }
    
    void setupRateIcon(std::unique_ptr<TextIcon>& icon, const juce::String& text, Params::RateOption rate, int groupIndex)
    {
        icon = std::make_unique<TextIcon>(text, 27.0f, 20.0f);
        icon->setNormalColour(juce::Colours::lightgrey);
        icon->setTooltip("Toggle " + text + " rate for Group " + juce::String(groupIndex + 1));
        
        // Add click handler
        icon->onClicked = [this, rate, groupIndex]() {
            // TODO: Implement group rate toggling in SampleManager
            bool currentState = processor.getSampleManager().isGroupRateEnabled(groupIndex, rate);
            processor.getSampleManager().setGroupRateEnabled(groupIndex, rate, !currentState);
            
            // Update icon state
            updateRateIconState(getGroupRateIcon(groupIndex, rate), groupIndex, rate);
        };
        
        addAndMakeVisible(icon.get());
    }
    
    void setupEffectIcon(std::unique_ptr<TextIcon>& icon, const juce::String& text, int groupIndex, EffectType effectType)
    {
        icon = std::make_unique<TextIcon>(text, 20.0f);
        icon->setNormalColour(juce::Colours::lightgrey);
        
        juce::String effectName;
        switch (effectType) {
            case EffectType::Reverb: effectName = "Reverb"; break;
            case EffectType::Stutter: effectName = "Stutter"; break;
            case EffectType::Delay: effectName = "Delay"; break;
        }
        
        icon->setTooltip("Allow " + effectName + " effect for Group " + juce::String(groupIndex + 1));
        
        // Add click handler
        icon->onClicked = [this, effectType, groupIndex]() {
            bool currentState = processor.getSampleManager().isGroupEffectEnabled(groupIndex, static_cast<int>(effectType));
            processor.getSampleManager().setGroupEffectEnabled(groupIndex, static_cast<int>(effectType), !currentState);
            
            // Update icon state
            updateEffectIconState(getGroupEffectIcon(groupIndex, effectType), groupIndex, effectType);
        };
        
        addAndMakeVisible(icon.get());
    }
    
    TextIcon* getGroupRateIcon(int groupIndex, Params::RateOption rate)
    {
        if (groupIndex < 0 || groupIndex >= MAX_GROUPS) return nullptr;
        
        switch (rate) {
            case Params::RATE_1_2: return rateIcons1_2[groupIndex].get();
            case Params::RATE_1_4: return rateIcons1_4[groupIndex].get();
            case Params::RATE_1_8: return rateIcons1_8[groupIndex].get();
            case Params::RATE_1_16: return rateIcons1_16[groupIndex].get();
            default: return nullptr;
        }
    }
    
    TextIcon* getGroupEffectIcon(int groupIndex, EffectType effectType)
    {
        if (groupIndex < 0 || groupIndex >= MAX_GROUPS) return nullptr;
        
        switch (effectType) {
            case EffectType::Reverb: return reverbIcons[groupIndex].get();
            case EffectType::Stutter: return stutterIcons[groupIndex].get();
            case EffectType::Delay: return delayIcons[groupIndex].get();
            default: return nullptr;
        }
    }
    
    void updateRateIconState(TextIcon* icon, int groupIndex, Params::RateOption rate)
    {
        if (icon == nullptr) return;
        
        bool isEnabled = processor.getSampleManager().isGroupRateEnabled(groupIndex, rate);
        if (isEnabled) {
            icon->setActive(true, juce::Colour(0xff52bfd9));
        } else {
            icon->setActive(false);
        }
    }
    
    void updateEffectIconState(TextIcon* icon, int groupIndex, EffectType effectType)
    {
        if (icon == nullptr) return;
        
        bool isEnabled = processor.getSampleManager().isGroupEffectEnabled(groupIndex, static_cast<int>(effectType));
        if (isEnabled) {
            icon->setActive(true, juce::Colour(0xff52bfd9));
        } else {
            icon->setActive(false);
        }
    }
}; 