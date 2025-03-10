#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../../Audio/PluginProcessor.h"
#include "Icon.h"
#include "../Sections/BaseSection.h"

class GroupListView
        : public juce::Component, public juce::Slider::Listener, public juce::Timer {
public:
    explicit GroupListView(PluginProcessor &p)
            : processor(p) {
        // Group title
        titleLabel.setText("SAMPLE GROUPS", juce::dontSendNotification);
        titleLabel.setFont(juce::Font(juce::FontOptions(TITLE_FONT_SIZE, juce::Font::bold)));
        titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff999999));
        titleLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(titleLabel);

        // Set title visibility - we'll control this externally
        titleLabel.setVisible(false);

        // Make the sliders and controls for each group
        for (int i = 0; i < MAX_GROUPS; ++i) {
            // Create the probability label
            auto &probLabel = probabilityLabels[i];
            probLabel = std::make_unique<juce::Label>();
            probLabel->setText("PROB", juce::dontSendNotification);
            probLabel->setFont(juce::Font(11.0f));
            probLabel->setColour(juce::Label::textColourId, juce::Colours::white);
            probLabel->setJustificationType(juce::Justification::centred);
            probLabel->setEnabled(false); // Initially disabled
            addAndMakeVisible(probLabel.get());

            auto &slider = probabilitySliders[i];
            slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                                    juce::Slider::TextBoxBelow);
            slider->setRange(0.0, 100.0, 1.0); // Range 0-100 instead of 0-1
            slider->setValue(100.0); // Default to 100%
            slider->setDoubleClickReturnValue(true, 100.0);
            slider->setColour(juce::Slider::rotarySliderFillColourId, getGroupColor(i));
            slider->setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff333333));
            slider->setColour(juce::Slider::thumbColourId, juce::Colours::white);
            slider->setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
            slider->setColour(juce::Slider::textBoxBackgroundColourId,
                              juce::Colours::transparentBlack); // Transparent background
            slider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack); // Remove outline
            slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
            slider->setNumDecimalPlacesToDisplay(0); // No decimal places for 0-100 range
            slider->setEnabled(false); // Initially disabled
            slider->addListener(this);
            slider->setTooltip("Group probability (0-100%)");
            slider->setLookAndFeel(&customLookAndFeel); // Use custom look and feel
            addAndMakeVisible(slider.get());

            auto &label = groupLabels[i];
            label = std::make_unique<juce::Label>();
            label->setText("GROUP " + juce::String(i + 1), juce::dontSendNotification);
            label->setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
            label->setColour(juce::Label::textColourId, getGroupColor(i).withAlpha(0.8f));
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

    void resized() override {
        auto bounds = getLocalBounds();

        // Title at the top (only takes space if visible)
        if (titleLabel.isVisible()) {
            titleLabel.setBounds(bounds.removeFromTop(24));
        }

        // Lay out the groups in a single row
        const int numGroups = juce::jmin(static_cast<int>(processor.getSampleManager().getNumGroups()), MAX_GROUPS);

        if (numGroups > 0) {
            // Calculate group width based on all groups in a single row
            const int groupWidth = bounds.getWidth() / numGroups;

            for (int i = 0; i < numGroups; ++i) {
                juce::Rectangle<int> groupBounds(
                        bounds.getX() + i * groupWidth,
                        bounds.getY(),
                        groupWidth,
                        bounds.getHeight()
                );

                // Store background area
                groupBackgrounds[i] = groupBounds.reduced(3);

                // Group label at the top
                groupLabels[i]->setBounds(groupBounds.removeFromTop(24));

                // Calculate space for all controls
                const int sliderSize = 48;
                const int rateIconWidth = 27;
                const int iconSize = 24;
                const int padding = 8;

                // Determine vertical positions
                // Start with effect icons at the top
                const int effectIconsY = groupLabels[i]->getBottom() + padding;

                // Position effect icons in a row
                int effectX = groupBounds.getCentreX() - ((iconSize * 3 + padding * 2) / 2);

                reverbIcons[i]->setBounds(effectX, effectIconsY, iconSize, iconSize);
                effectX += iconSize + padding;
                stutterIcons[i]->setBounds(effectX, effectIconsY, iconSize, iconSize);
                effectX += iconSize + padding;
                delayIcons[i]->setBounds(effectX, effectIconsY, iconSize, iconSize);

                // Position probability label below the effect icons
                juce::Rectangle<int> probLabelArea = groupBounds.removeFromTop(18);
                probLabelArea.setY(effectIconsY + iconSize + padding);
                probabilityLabels[i]->setBounds(probLabelArea);

                // Position the slider below the probability label
                juce::Rectangle<int> sliderArea = groupBounds.withSizeKeepingCentre(sliderSize, sliderSize);
                sliderArea.setY(probLabelArea.getBottom() + padding);
                probabilitySliders[i]->setBounds(sliderArea);

                // Add a "RATE" label above the rate icons to make them more obvious
                auto &rateLabel = rateLabels[i];
                if (!rateLabel) {
                    rateLabel = std::make_unique<juce::Label>();
                    rateLabel->setText("RATE", juce::dontSendNotification);
                    rateLabel->setFont(juce::Font(11.0f));
                    rateLabel->setColour(juce::Label::textColourId, juce::Colours::white);
                    rateLabel->setJustificationType(juce::Justification::centred);
                    addAndMakeVisible(rateLabel.get());
                }
                
                int rateLabelY = sliderArea.getBottom() + padding;
                rateLabel->setBounds(groupBounds.getX(), rateLabelY, groupBounds.getWidth(), 16);

                // Position rate icons in a row below the label
                int rateY = rateLabelY + 16 + 4; // Below the rate label with a small gap
                int rateX = groupBounds.getCentreX() - ((rateIconWidth * 4 + padding * 3) / 2);

                rateIcons1_2[i]->setBounds(rateX, rateY, rateIconWidth, iconSize);
                rateX += rateIconWidth + padding;
                rateIcons1_4[i]->setBounds(rateX, rateY, rateIconWidth, iconSize);
                rateX += rateIconWidth + padding;
                rateIcons1_8[i]->setBounds(rateX, rateY, rateIconWidth, iconSize);
                rateX += rateIconWidth + padding;
                rateIcons1_16[i]->setBounds(rateX, rateY, rateIconWidth, iconSize);
            }
        }
    }

    void sliderValueChanged(juce::Slider *slider) override {
        // Find which slider changed
        for (int i = 0; i < MAX_GROUPS; ++i) {
            if (slider == probabilitySliders[i].get()) {
                // Convert from 0-100 range to 0-1 range for the SampleManager
                float normalizedValue = static_cast<float>(slider->getValue()) / 100.0f;
                // Update the group probability
                processor.getSampleManager().setGroupProbability(i, normalizedValue);
                break;
            }
        }
    }

    void timerCallback() override {
        // Check if the number of groups has changed
        const int numGroups = processor.getSampleManager().getNumGroups();

        if (numGroups != lastNumGroups) {
            lastNumGroups = numGroups;

            // Update the UI
            for (int i = 0; i < MAX_GROUPS; ++i) {
                bool groupExists = (i < numGroups);
                probabilitySliders[i]->setEnabled(groupExists);
                probabilityLabels[i]->setEnabled(groupExists);
                groupLabels[i]->setEnabled(groupExists);
                
                // Also update the rate label visibility
                if (rateLabels[i] != nullptr) {
                    rateLabels[i]->setVisible(groupExists);
                }

                // Enable/disable all rate and effect icons
                rateIcons1_2[i]->setVisible(groupExists);
                rateIcons1_4[i]->setVisible(groupExists);
                rateIcons1_8[i]->setVisible(groupExists);
                rateIcons1_16[i]->setVisible(groupExists);

                reverbIcons[i]->setVisible(groupExists);
                stutterIcons[i]->setVisible(groupExists);
                delayIcons[i]->setVisible(groupExists);

                if (groupExists) {
                    // Update group name and probability
                    if (const SampleManager::Group *group = processor.getSampleManager().getGroup(i)) {
                        groupLabels[i]->setText(group->name, juce::dontSendNotification);
                        // Convert from 0-1 range to 0-100 range for display
                        probabilitySliders[i]->setValue(group->probability * 100.0, juce::dontSendNotification);

                        // Update rate icon states
                        updateRateIconState(rateIcons1_2[i].get(), i, Params::RATE_1_2);
                        updateRateIconState(rateIcons1_4[i].get(), i, Params::RATE_1_4);
                        updateRateIconState(rateIcons1_8[i].get(), i, Params::RATE_1_8);
                        updateRateIconState(rateIcons1_16[i].get(), i, Params::RATE_1_16);

                        // Update effect icon states
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
    void setTitleVisible(bool shouldBeVisible) {
        titleLabel.setVisible(shouldBeVisible);
    }

    void paint(juce::Graphics &g) override {
        // Draw a metallic panel background like in the BaseSection
        auto bounds = getLocalBounds();
        auto baseColor = juce::Colour(0xff2a2a2a);

        // Draw the panel background
        g.setGradientFill(juce::ColourGradient(baseColor.brighter(0.1f),
                                               bounds.getX(),
                                               bounds.getY(),
                                               baseColor.darker(0.1f),
                                               bounds.getX(),
                                               bounds.getBottom(),
                                               false));
        g.fillRect(bounds);

        // Draw a subtle inner shadow
        g.setColour(juce::Colour(0x20000000));
        g.drawRect(bounds.expanded(1, 1), 2);

        // Draw a highlight on the top edge
        g.setColour(juce::Colour(0x30ffffff));
        g.drawLine(bounds.getX() + 2,
                   bounds.getY() + 1,
                   bounds.getRight() - 2,
                   bounds.getY() + 1,
                   1.0f);

        // Draw accent line under title if visible
        if (titleLabel.isVisible()) {
            g.setColour(juce::Colour(0xff999999).withAlpha(0.5f));
            g.drawLine(10, titleLabel.getBottom() + 5, getWidth() - 10, titleLabel.getBottom() + 5, 1.0f);
        }

        // Draw backgrounds for each active group
        const int numGroups = processor.getSampleManager().getNumGroups();
        
        // Draw vertical divider lines between groups
        if (numGroups > 1) {
            g.setColour(juce::Colour(0x40999999));
            const int groupWidth = bounds.getWidth() / numGroups;
            
            for (int i = 1; i < numGroups && i < MAX_GROUPS; ++i) {
                int dividerX = i * groupWidth;
                g.drawLine(dividerX, titleLabel.isVisible() ? titleLabel.getBottom() + 10 : bounds.getY() + 10, 
                           dividerX, bounds.getBottom() - 10, 1.0f);
            }
        }
        
        // We no longer need to draw black backgrounds for groups, as we want the metallic panel to show through
        for (int i = 0; i < numGroups && i < MAX_GROUPS; ++i) {
            // Draw a subtle border with the group color
            g.setColour(getGroupColor(i).withAlpha(0.7f));
            g.drawRoundedRectangle(groupBackgrounds[i].toFloat(), 4.0f, 1.0f);
            
            // Draw a darker background for the rate icons section to make it stand out
            // Only if the rate label exists and is visible
            if (rateLabels[i] != nullptr && rateLabels[i]->isVisible()) {
                int rateY = rateLabels[i]->getY() - 4; // Start slightly above the label
                int rateHeight = 24 + 16 + 8; // Height of label + icons + padding
                
                juce::Rectangle<float> rateArea(
                    groupBackgrounds[i].getX() + 4, 
                    rateY,
                    groupBackgrounds[i].getWidth() - 8, 
                    rateHeight
                );
                
                g.setColour(juce::Colour(0x20000000)); // Slightly darker than background
                g.fillRoundedRectangle(rateArea, 3.0f);
                
                // Draw a subtle border around rate area with the group color
                g.setColour(getGroupColor(i).withAlpha(0.3f));
                g.drawRoundedRectangle(rateArea, 3.0f, 0.5f);
            }
        }
    }

private:
    // Custom look and feel for the sliders to match the plugin style
    class GroupSliderLookAndFeel : public juce::LookAndFeel_V4 {
    public:
        GroupSliderLookAndFeel() {
            setColour(juce::Slider::thumbColourId, juce::Colours::white);
        }

        void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                              float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                              juce::Slider &slider) override {
            // Get fill color from the slider
            juce::Colour fillColor = slider.findColour(juce::Slider::rotarySliderFillColourId);

            float radius = juce::jmin(width / 2.0f, height / 2.0f) - 4.0f;
            float centerX = x + width * 0.5f;
            float centerY = y + height * 0.5f;
            float rx = centerX - radius;
            float ry = centerY - radius;
            float rw = radius * 2.0f;

            // Map slider value to angle
            float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

            // Background circle (black to match the screenshot)
            g.setColour(juce::Colours::black);
            g.fillEllipse(rx, ry, rw, rw);

            // Outline (dark grey)
            g.setColour(juce::Colour(0xff444444));
            g.drawEllipse(rx, ry, rw, rw, 1.0f);

            // Draw the filled arc
            juce::Path filledArc;
            filledArc.addPieSegment(rx, ry, rw, rw, rotaryStartAngle, angle, 0.0f);
            g.setColour(fillColor);
            g.fillPath(filledArc);

            // Draw marker dot in center
            float dotSize = radius * 0.2f;
            g.setColour(fillColor);
            g.fillEllipse(centerX - dotSize, centerY - dotSize, dotSize * 2.0f, dotSize * 2.0f);

            // Draw line indicator
            juce::Path markerLine;
            float lineThickness = 2.0f;
            markerLine.addRectangle(-lineThickness * 0.5f, -radius + 2.0f, lineThickness, radius * 0.6f);

            g.setColour(juce::Colours::white);
            g.fillPath(markerLine, juce::AffineTransform::rotation(angle).translated(centerX, centerY));
        }
    };

    enum class EffectType {
        Reverb, Stutter, Delay
    };

    PluginProcessor &processor;
    juce::Label titleLabel;
    GroupSliderLookAndFeel customLookAndFeel;

    static constexpr int MAX_GROUPS = 4;
    std::unique_ptr<juce::Slider> probabilitySliders[MAX_GROUPS];
    std::unique_ptr<juce::Label> groupLabels[MAX_GROUPS];
    std::unique_ptr<juce::Label> probabilityLabels[MAX_GROUPS];
    juce::Rectangle<int> groupBackgrounds[MAX_GROUPS];

    // Rate icons (1/2, 1/4, 1/8, 1/16) for each group
    std::unique_ptr<TextIcon> rateIcons1_2[MAX_GROUPS];
    std::unique_ptr<TextIcon> rateIcons1_4[MAX_GROUPS];
    std::unique_ptr<TextIcon> rateIcons1_8[MAX_GROUPS];
    std::unique_ptr<TextIcon> rateIcons1_16[MAX_GROUPS];

    // Effect icons (reverb, stutter, delay) for each group
    std::unique_ptr<TextIcon> reverbIcons[MAX_GROUPS];
    std::unique_ptr<TextIcon> stutterIcons[MAX_GROUPS];
    std::unique_ptr<TextIcon> delayIcons[MAX_GROUPS];

    // Add a rate label for each group to make the rate section more obvious
    std::unique_ptr<juce::Label> rateLabels[MAX_GROUPS];

    int lastNumGroups = 0;

    juce::Colour getGroupColor(int index) const {
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

    void
    setupRateIcon(std::unique_ptr<TextIcon> &icon, const juce::String &text, Params::RateOption rate, int groupIndex) {
        icon = std::make_unique<TextIcon>(text, 27.0f, 24.0f);
        
        // Make icons more visible with brighter initial color
        icon->setNormalColour(juce::Colour(0xffaaaaaa));
        icon->setTooltip("Toggle " + text + " rate for Group " + juce::String(groupIndex + 1));

        // Add click handler
        icon->onClicked = [this, rate, groupIndex]() {
            bool currentState = processor.getSampleManager().isGroupRateEnabled(groupIndex, rate);
            processor.getSampleManager().setGroupRateEnabled(groupIndex, rate, !currentState);

            // Update icon state
            updateRateIconState(getGroupRateIcon(groupIndex, rate), groupIndex, rate);
        };

        addAndMakeVisible(icon.get());
    }

    void
    setupEffectIcon(std::unique_ptr<TextIcon> &icon, const juce::String &text, int groupIndex, EffectType effectType) {
        icon = std::make_unique<TextIcon>(text, 24.0f, 24.0f);
        icon->setNormalColour(juce::Colour(0xff888888));

        // Set tooltip based on effect type
        juce::String effectName;
        switch (effectType) {
            case EffectType::Reverb:
                effectName = "Reverb";
                break;
            case EffectType::Stutter:
                effectName = "Stutter";
                break;
            case EffectType::Delay:
                effectName = "Delay";
                break;
        }

        icon->setTooltip("Toggle " + effectName + " for Group " + juce::String(groupIndex + 1));

        // Add click handler
        icon->onClicked = [this, effectType, groupIndex]() {
            // TODO: Implement effect toggling in SampleManager
            int effectTypeIdx = static_cast<int>(effectType);
            bool currentState = processor.getSampleManager().isGroupEffectEnabled(groupIndex, effectTypeIdx);
            processor.getSampleManager().setGroupEffectEnabled(groupIndex, effectTypeIdx, !currentState);

            // Update icon state
            updateEffectIconState(getGroupEffectIcon(groupIndex, effectType), groupIndex, effectType);
        };

        addAndMakeVisible(icon.get());
    }

    TextIcon *getGroupRateIcon(int groupIndex, Params::RateOption rate) {
        if (groupIndex < 0 || groupIndex >= MAX_GROUPS) return nullptr;

        switch (rate) {
            case Params::RATE_1_2:
                return rateIcons1_2[groupIndex].get();
            case Params::RATE_1_4:
                return rateIcons1_4[groupIndex].get();
            case Params::RATE_1_8:
                return rateIcons1_8[groupIndex].get();
            case Params::RATE_1_16:
                return rateIcons1_16[groupIndex].get();
            default:
                return nullptr;
        }
    }

    TextIcon *getGroupEffectIcon(int groupIndex, EffectType effectType) {
        if (groupIndex < 0 || groupIndex >= MAX_GROUPS) return nullptr;

        switch (effectType) {
            case EffectType::Reverb:
                return reverbIcons[groupIndex].get();
            case EffectType::Stutter:
                return stutterIcons[groupIndex].get();
            case EffectType::Delay:
                return delayIcons[groupIndex].get();
            default:
                return nullptr;
        }
    }

    void updateRateIconState(TextIcon *icon, int groupIndex, Params::RateOption rate) {
        if (icon == nullptr) return;

        bool isEnabled = processor.getSampleManager().isGroupRateEnabled(groupIndex, rate);
        
        // Use group color for active state to make it more distinct
        if (isEnabled) {
            icon->setActive(true, getGroupColor(groupIndex));
        } else {
            icon->setActive(false);
        }
    }

    void updateEffectIconState(TextIcon *icon, int groupIndex, EffectType effectType) {
        if (icon == nullptr)
            return;

        int effectTypeIdx = static_cast<int>(effectType);
        bool isEnabled = processor.getSampleManager().isGroupEffectEnabled(groupIndex, effectTypeIdx);

        if (isEnabled)
            icon->setActive(true, getGroupColor(groupIndex));
        else
            icon->setActive(false);
    }
}; 