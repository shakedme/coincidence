#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../../../Audio/PluginProcessor.h"
#include "../Icon.h"
#include "../../Sections/BaseSection.h"

class GroupListView
        : public juce::Component, public juce::Timer {
public:
    explicit GroupListView(PluginProcessor &p)
            : processor(p) {

        // Make the sliders and controls for each group
        for (int i = 0; i < MAX_GROUPS; ++i) {
            auto &slider = probabilitySliders[i];
            slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                                    juce::Slider::TextBoxBelow);
            slider->setRange(0.0, 100.0, 1.0);
            slider->setValue(100.0);
            slider->setDoubleClickReturnValue(true, 100.0);
            slider->setNumDecimalPlacesToDisplay(0);
            slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 50, 6);
            slider->setTooltip("Group probability (0-100%)");
            slider->setColour(juce::Slider::rotarySliderFillColourId, getGroupColor(i));
            slider->setColour(juce::Slider::thumbColourId, getGroupColor(i));
            slider->onValueChange = [&]() {
                float normalizedValue = static_cast<float>(slider->getValue()) / 100.0f;
                processor.getSampleManager().setGroupProbability(i, normalizedValue);
            };
            addAndMakeVisible(slider.get());

            // Group title label
            auto &label = groupLabels[i];
            label = std::make_unique<juce::Label>();
            label->setText("GROUP " + juce::String(i + 1), juce::dontSendNotification);
            label->setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
            label->setColour(juce::Label::textColourId, getGroupColor(i).withAlpha(0.8f));
            label->setJustificationType(juce::Justification::centred);
            label->setEnabled(false); // Initially disabled
            addAndMakeVisible(label.get());

            // Create rate section labels
            auto &rateLabel = rateLabels[i];
            rateLabel = std::make_unique<juce::Label>();
            rateLabel->setText("RATES", juce::dontSendNotification);
            rateLabel->setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
            rateLabel->setColour(juce::Label::textColourId, getGroupColor(i).withAlpha(0.8f));
            rateLabel->setJustificationType(juce::Justification::centred);
            rateLabel->setEnabled(false); // Initially disabled
            addAndMakeVisible(rateLabel.get());

            // Create rate icons 
            setupRateIcon(rateIcons1_1[i], "1/1", Models::RATE_1_1, i);
            setupRateIcon(rateIcons1_2[i], "1/2", Models::RATE_1_2, i);
            setupRateIcon(rateIcons1_4[i], "1/4", Models::RATE_1_4, i);
            setupRateIcon(rateIcons1_8[i], "1/8", Models::RATE_1_8, i);
            setupRateIcon(rateIcons1_16[i], "1/16", Models::RATE_1_16, i);
            setupRateIcon(rateIcons1_32[i], "1/32", Models::RATE_1_32, i);

            // Initially hide components (will be shown in resized() if group is active)
            label->setVisible(false);
            slider->setVisible(false);
            rateLabel->setVisible(false);
            rateIcons1_1[i]->setVisible(false);
            rateIcons1_2[i]->setVisible(false);
            rateIcons1_4[i]->setVisible(false);
            rateIcons1_8[i]->setVisible(false);
            rateIcons1_16[i]->setVisible(false);
            rateIcons1_32[i]->setVisible(false);
        }

        // Set initial size
        setSize(300, 200);

        // Start a timer to periodically update the UI
        startTimerHz(5);
    }

    ~GroupListView() override = default;

    void resized() override {
        auto bounds = getLocalBounds();

        // Get the number of active groups
        const int numGroups = static_cast<int>(processor.getSampleManager().getNumGroups());

        // Skip layout if no groups exist
        if (numGroups == 0) {
            return;
        }

        // Calculate fixed group width (each group takes 1/8 of the total width)
        const int groupWidth = bounds.getWidth() / MAX_GROUPS;
        const int padding = 4; // Smaller padding to save space
        const int sliderSize = 30;
        const int labelHeight = 15;

        // Layout all potential group slots
        for (int i = 0; i < MAX_GROUPS; ++i) {
            // Get bounds for this group position (whether active or not)
            juce::Rectangle<int> workingBounds(
                    bounds.getX() + i * groupWidth,
                    bounds.getY(),
                    groupWidth,
                    bounds.getHeight()
            );

            // Only set up actual components for active groups
            if (i < numGroups) {
                // Group title
                groupLabels[i]->setBounds(workingBounds.getX(), workingBounds.getY(), workingBounds.getWidth(),
                                          labelHeight);

                probabilitySliders[i]->setBounds(
                        workingBounds.getX() + workingBounds.getWidth() / 2 - sliderSize / 2,
                        workingBounds.getY() + labelHeight + padding,
                        sliderSize,
                        sliderSize
                );


                // Position effects in a row
                const int rateLabelsY = probabilitySliders[i]->getY() + labelHeight;

                // Rate section label at the top
                rateLabels[i]->setBounds(
                        workingBounds.getX(),
                        rateLabelsY + rateIconHeight,
                        workingBounds.getWidth(),
                        labelHeight
                );

                // Position rate icons in a grid layout - 2 rows of 3 icons
                const int ratesY1 = rateLabels[i]->getY() + labelHeight;
                const int ratesY2 = ratesY1 + rateIconHeight;
                const int ratesSpacing = workingBounds.getWidth() / 3;

                // First row
                rateIcons1_1[i]->setBounds(
                        workingBounds.getX() + ratesSpacing / 2 - rateIconWidth / 2,
                        ratesY1,
                        rateIconWidth,
                        rateIconHeight
                );

                rateIcons1_2[i]->setBounds(
                        workingBounds.getX() + ratesSpacing * 1.5f - rateIconWidth / 2,
                        ratesY1,
                        rateIconWidth,
                        rateIconHeight
                );

                rateIcons1_4[i]->setBounds(
                        workingBounds.getX() + ratesSpacing * 2.5f - rateIconWidth / 2,
                        ratesY1,
                        rateIconWidth,
                        rateIconHeight
                );

                // Second row
                rateIcons1_8[i]->setBounds(
                        workingBounds.getX() + ratesSpacing / 2 - rateIconWidth / 2,
                        ratesY2,
                        rateIconWidth,
                        rateIconHeight
                );

                rateIcons1_16[i]->setBounds(
                        workingBounds.getX() + ratesSpacing * 1.5f - rateIconWidth / 2,
                        ratesY2,
                        rateIconWidth,
                        rateIconHeight
                );

                rateIcons1_32[i]->setBounds(
                        workingBounds.getX() + ratesSpacing * 2.5f - rateIconWidth / 2,
                        ratesY2,
                        rateIconWidth,
                        rateIconHeight
                );

                // Ensure the components are visible for active groups
                groupLabels[i]->setVisible(true);
                probabilitySliders[i]->setVisible(true);
                rateLabels[i]->setVisible(true);
                rateIcons1_1[i]->setVisible(true);
                rateIcons1_2[i]->setVisible(true);
                rateIcons1_4[i]->setVisible(true);
                rateIcons1_8[i]->setVisible(true);
                rateIcons1_16[i]->setVisible(true);
                rateIcons1_32[i]->setVisible(true);
            } else {
                // Hide components for inactive groups
                if (groupLabels[i]) groupLabels[i]->setVisible(false);
                if (probabilitySliders[i]) probabilitySliders[i]->setVisible(false);
                if (rateLabels[i]) rateLabels[i]->setVisible(false);
                if (rateIcons1_1[i]) rateIcons1_1[i]->setVisible(false);
                if (rateIcons1_2[i]) rateIcons1_2[i]->setVisible(false);
                if (rateIcons1_4[i]) rateIcons1_4[i]->setVisible(false);
                if (rateIcons1_8[i]) rateIcons1_8[i]->setVisible(false);
                if (rateIcons1_16[i]) rateIcons1_16[i]->setVisible(false);
                if (rateIcons1_32[i]) rateIcons1_32[i]->setVisible(false);
            }
        }
    }

    void timerCallback() override {
        // Get the current number of groups
        const int numGroups = processor.getSampleManager().getNumGroups();

        // Check if the number of groups changed
        if (numGroups != lastNumGroups) {
            lastNumGroups = numGroups;

            // Enable/disable components based on how many groups exist
            for (int i = 0; i < MAX_GROUPS; ++i) {
                bool groupExists = i < numGroups;

                // Components might not be created yet if processor is still initializing
                if (groupLabels[i] && probabilitySliders[i]) {
                    groupLabels[i]->setEnabled(groupExists);
                    probabilitySliders[i]->setEnabled(groupExists);
                }
            }
        }

        // Update all active groups
        for (int i = 0; i < numGroups && i < MAX_GROUPS; ++i) {
            // Components might not be created yet if processor is still initializing
            if (groupLabels[i] && probabilitySliders[i]) {
                bool groupExists = i < numGroups;

                if (groupExists) {
                    // Update group name and probability
                    if (const SampleManager::Group *group = processor.getSampleManager().getGroup(i)) {
                        groupLabels[i]->setText(group->name, juce::dontSendNotification);
                        // Convert from 0-1 range to 0-100 range for display
                        probabilitySliders[i]->setValue(group->probability * 100.0, juce::dontSendNotification);

                        // Update rate icon states
                        updateRateIconState(rateIcons1_1[i].get(), i, Models::RATE_1_1);
                        updateRateIconState(rateIcons1_2[i].get(), i, Models::RATE_1_2);
                        updateRateIconState(rateIcons1_4[i].get(), i, Models::RATE_1_4);
                        updateRateIconState(rateIcons1_8[i].get(), i, Models::RATE_1_8);
                        updateRateIconState(rateIcons1_16[i].get(), i, Models::RATE_1_16);
                        updateRateIconState(rateIcons1_32[i].get(), i, Models::RATE_1_32);
                    }
                }
            }
        }

        // Resize to update the layout
        resized();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds();

        // Get the number of active groups
        const int numGroups = static_cast<int>(processor.getSampleManager().getNumGroups());

        if (numGroups > 0) {
            // Draw vertical divider lines between all 8 group slots when groups exist
            const int groupWidth = bounds.getWidth() / MAX_GROUPS;
            g.setColour(juce::Colour(0xffbf52d9).withAlpha(0.5f));
            for (int i = 1; i < MAX_GROUPS; ++i) {
                int dividerX = i * groupWidth;
                g.drawLine(dividerX,
                           bounds.getY(),
                           dividerX,
                           bounds.getBottom() - 10,
                           1.0f);
            }
        } else {
            // Display centered message when no groups exist
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            g.setFont(juce::Font(juce::FontOptions(14.0f)));
            g.drawText("Create groups by multi-selecting samples and right clicking",
                       bounds,
                       juce::Justification::centred,
                       true);
        }
    }

private:
    PluginProcessor &processor;
    juce::Label titleLabel;

    static constexpr int rateIconWidth = 30;
    static constexpr int rateIconHeight = 15;
    static constexpr int MAX_GROUPS = 8;

    std::unique_ptr<juce::Slider> probabilitySliders[MAX_GROUPS];
    std::unique_ptr<juce::Label> groupLabels[MAX_GROUPS];

    std::unique_ptr<TextIcon> rateIcons1_1[MAX_GROUPS];
    std::unique_ptr<TextIcon> rateIcons1_2[MAX_GROUPS];
    std::unique_ptr<TextIcon> rateIcons1_4[MAX_GROUPS];
    std::unique_ptr<TextIcon> rateIcons1_8[MAX_GROUPS];
    std::unique_ptr<TextIcon> rateIcons1_16[MAX_GROUPS];
    std::unique_ptr<TextIcon> rateIcons1_32[MAX_GROUPS];

    std::unique_ptr<juce::Label> rateLabels[MAX_GROUPS];

    int lastNumGroups = 0;

    juce::Colour getGroupColor(int index) const {
        // Different colors for each group
        const juce::Colour colors[MAX_GROUPS] = {
                juce::Colour(0xff5c9ce6), // Blue
                juce::Colour(0xff52bf5d), // Green
                juce::Colour(0xffbf5252), // Red
                juce::Colour(0xffbf52d9),  // Purple
                juce::Colour(0xff52bfbf),  // Cyan
                juce::Colour(0xff52d9bf),  // Light Cyan
                juce::Colour(0xffbf52bf),  // Magenta
                juce::Colour(0xffd952bf)   // Light Magenta
        };

        // Make sure index is within range
        const int numColors = sizeof(colors) / sizeof(colors[0]);
        if (index >= 0 && index < numColors)
            return colors[index];

        return juce::Colours::grey;
    }

    void
    setupRateIcon(std::unique_ptr<TextIcon> &icon, const juce::String &text, Models::RateOption rate, int groupIndex) {
        // Increase width from 27.0f to 35.0f to ensure text fits
        icon = std::make_unique<TextIcon>(text, rateIconWidth, rateIconHeight);

        // Make icons more visible with brighter initial color
        icon->setNormalColour(juce::Colour(0xffaaaaaa));
        icon->setTooltip("Toggle " + text + " rate for Group " + juce::String(groupIndex + 1) +
                         ". When disabled, the rate will never be played regardless of probability settings.\"");

        // Add click handler
        icon->onClicked = [this, rate, groupIndex]() {
            bool currentState = processor.getSampleManager().isGroupRateEnabled(groupIndex, rate);
            processor.getSampleManager().setGroupRateEnabled(groupIndex, rate, !currentState);

            // Update icon state
            updateRateIconState(getGroupRateIcon(groupIndex, rate), groupIndex, rate);
        };

        addAndMakeVisible(icon.get());
    }

    TextIcon *getGroupRateIcon(int groupIndex, Models::RateOption rate) {
        if (groupIndex < 0 || groupIndex >= MAX_GROUPS) return nullptr;

        switch (rate) {
            case Models::RATE_1_1:
                return rateIcons1_1[groupIndex].get();
            case Models::RATE_1_2:
                return rateIcons1_2[groupIndex].get();
            case Models::RATE_1_4:
                return rateIcons1_4[groupIndex].get();
            case Models::RATE_1_8:
                return rateIcons1_8[groupIndex].get();
            case Models::RATE_1_16:
                return rateIcons1_16[groupIndex].get();
            case Models::RATE_1_32:
                return rateIcons1_32[groupIndex].get();
            default:
                return nullptr;
        }
    }

    void updateRateIconState(TextIcon *icon, int groupIndex, Models::RateOption rate) {
        if (icon == nullptr) return;

        bool isEnabled = processor.getSampleManager().isGroupRateEnabled(groupIndex, rate);

        // Use group color for active state to make it more distinct
        if (isEnabled) {
            icon->setActive(true, getGroupColor(groupIndex));
        } else {
            icon->setActive(false);
        }

    }

}; 