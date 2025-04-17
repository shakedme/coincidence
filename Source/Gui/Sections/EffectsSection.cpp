//
// Created by Shaked Melman on 01/03/2025.
//

#include "EffectsSection.h"

EffectsSection::EffectsSection(PluginEditor &e, PluginProcessor &p)
        : BaseSectionComponent(e, p, "EFFECTS", juce::Colour(0xffd9a652)) {
    // Ensure subsections are positioned below horizontal divider
    firstRowY = 60; // Increased from 45 to create more space below the header
    
    const int compactKnobSize = 36;

    initKnob(stutterKnob, "Stutter", Params::ID_STUTTER_PROBABILITY, 0, 100, 0.1, "");
    initLabel(stutterLabel, "STUTTER");
    stutterKnob->setSize(compactKnobSize, compactKnobSize);

    stutterSectionLabel =
            std::unique_ptr<juce::Label>(createLabel("STUTTER", juce::Justification::centred));
    stutterSectionLabel->setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    stutterSectionLabel->setColour(juce::Label::textColourId,
                                   sectionColour.withAlpha(0.8f));
    addAndMakeVisible(stutterSectionLabel.get());

    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_STUTTER_PROBABILITY, *stutterKnob));

    reverbSectionLabel =
            std::unique_ptr<juce::Label>(createLabel("REVERB", juce::Justification::centred));
    reverbSectionLabel->setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    reverbSectionLabel->setColour(juce::Label::textColourId,
                                  sectionColour.withAlpha(0.8f));
    addAndMakeVisible(reverbSectionLabel.get());

    // Mix knob
    initKnob(reverbMixKnob, "Reverb Mix", Params::ID_REVERB_MIX, 0, 100, 0.1, "");
    initLabel(reverbMixLabel, "MIX");
    reverbMixKnob->setSize(compactKnobSize, compactKnobSize);

    // Time knob
    initKnob(reverbTimeKnob, "Reverb Time", Params::ID_REVERB_TIME, 0, 100, 0.1, "");
    initLabel(reverbTimeLabel, "TIME");
    reverbTimeKnob->setSize(compactKnobSize, compactKnobSize);

    // Width knob
    initKnob(reverbWidthKnob, "Reverb Width", Params::ID_REVERB_WIDTH, 0, 100, 0.1, "");
    initLabel(reverbWidthLabel, "WIDTH");
    reverbWidthKnob->setSize(compactKnobSize, compactKnobSize);

    // Parameter attachments for reverb
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_REVERB_MIX, *reverbMixKnob));

    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_REVERB_TIME, *reverbTimeKnob));

    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_REVERB_WIDTH, *reverbWidthKnob));

    // Delay section label
    delaySectionLabel =
            std::unique_ptr<juce::Label>(createLabel("DELAY", juce::Justification::centred));
    delaySectionLabel->setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    delaySectionLabel->setColour(juce::Label::textColourId,
                                 sectionColour.withAlpha(0.8f));
    addAndMakeVisible(delaySectionLabel.get());

    // Mix knob
    initKnob(delayMixKnob, "Delay Mix", Params::ID_DELAY_MIX, 0, 100, 0.1, "");
    initLabel(delayMixLabel, "MIX");
    delayMixKnob->setSize(compactKnobSize, compactKnobSize);

    // Rate knob - set initial tooltip based on BPM sync state
    initKnob(delayRateKnob, "Delay Rate", Params::ID_DELAY_RATE, 0, 100, 0.1, "");
    initLabel(delayRateLabel, "RATE");
    delayRateKnob->setSize(compactKnobSize, compactKnobSize);

    // Feedback knob
    initKnob(delayFeedbackKnob, "Delay Feedback", Params::ID_DELAY_FEEDBACK, 0, 100, 0.1, "");
    initLabel(delayFeedbackLabel, "FDBK");
    delayFeedbackKnob->setSize(compactKnobSize, compactKnobSize);

    // Add value listener to update tooltip when rate changes
    delayRateKnob->onValueChange = [this]() {
        updateDelayRateKnobTooltip();
    };

    // Ping Pong toggle
    delayPingPongToggle = std::make_unique<Toggle>(sectionColour);
    delayPingPongToggle->setTooltip("Toggle between ping pong mode and normal delay");
    delayPingPongToggle->setSize(28, 16);

    // Get the parameter directly
    auto *pingPongParam = dynamic_cast<juce::AudioParameterBool *>(
            processor.getAPVTS().getParameter(Params::ID_DELAY_PING_PONG));

    // Set initial value from parameter
    if (pingPongParam)
        delayPingPongToggle->setValue(pingPongParam->get());

    // Direct connection to parameter
    delayPingPongToggle->onValueChanged = [this](bool newValue) {
        auto *param = processor.getAPVTS().getParameter(Params::ID_DELAY_PING_PONG);
        if (param) {
            param->beginChangeGesture();
            param->setValueNotifyingHost(newValue ? 1.0f : 0.0f);
            param->endChangeGesture();
        }
        updatePingPongTooltip();
    };

    addAndMakeVisible(delayPingPongToggle.get());

    // BPM Sync toggle
    delayBpmSyncToggle = std::make_unique<Toggle>(sectionColour);
    delayBpmSyncToggle->setTooltip("Toggle between BPM sync and milliseconds");
    delayBpmSyncToggle->setSize(28, 16);

    // Get the parameter directly
    auto *bpmSyncParam = dynamic_cast<juce::AudioParameterBool *>(
            processor.getAPVTS().getParameter(Params::ID_DELAY_BPM_SYNC));

    // Set initial value from parameter
    if (bpmSyncParam)
        delayBpmSyncToggle->setValue(bpmSyncParam->get());

    // Direct connection to parameter
    delayBpmSyncToggle->onValueChanged = [this](bool newValue) {
        auto *param = processor.getAPVTS().getParameter(Params::ID_DELAY_BPM_SYNC);
        if (param) {
            param->beginChangeGesture();
            param->setValueNotifyingHost(newValue ? 1.0f : 0.0f);
            param->endChangeGesture();

            // For BPM sync, set snap-to-grid intervals for discrete note values
            if (newValue) {
                delayRateKnob->setNumDecimalPlacesToDisplay(0);
                delayRateKnob->setRange(0, 100, 20); // Snap to steps: 0, 20, 40, 60, 80, 100
            } else {
                delayRateKnob->setRange(0, 100, 0.1); // Smooth range for ms
            }
        }
        updateBpmSyncTooltip();
    };

    addAndMakeVisible(delayBpmSyncToggle.get());

    // Update tooltips
    updateDelayRateKnobTooltip();
    updatePingPongTooltip();
    updateBpmSyncTooltip();

    // Parameter attachments for delay
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_DELAY_MIX, *delayMixKnob));

    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_DELAY_RATE, *delayRateKnob));

    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_DELAY_FEEDBACK, *delayFeedbackKnob));

    // Compression section label
    compSectionLabel =
            std::unique_ptr<juce::Label>(createLabel("COMPRESSION", juce::Justification::centred));
    compSectionLabel->setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    compSectionLabel->setColour(juce::Label::textColourId,
                                  sectionColour.withAlpha(0.8f));
    addAndMakeVisible(compSectionLabel.get());

    // Mix knob
    initKnob(compMixKnob, "Compression Mix", Params::ID_COMPRESSION_MIX, 0, 100, 0.1, "");
    initLabel(compMixLabel, "MIX");
    compMixKnob->setSize(compactKnobSize, compactKnobSize);

    // Threshold knob
    initKnob(compThresholdKnob, "Compression Threshold", Params::ID_COMPRESSION_THRESHOLD, -60, 0, 0.1, "dB");
    initLabel(compThresholdLabel, "THRESH");
    compThresholdKnob->setSize(compactKnobSize, compactKnobSize);

    // Ratio knob
    initKnob(compRatioKnob, "Compression Ratio", Params::ID_COMPRESSION_RATIO, 1, 20, 0.1, ":1");
    initLabel(compRatioLabel, "RATIO");
    compRatioKnob->setSize(compactKnobSize, compactKnobSize);

    // Attack knob
    initKnob(compAttackKnob, "Compression Attack", Params::ID_COMPRESSION_ATTACK, 0.1, 100, 0.1, "ms");
    initLabel(compAttackLabel, "ATTACK");
    compAttackKnob->setSize(compactKnobSize, compactKnobSize);

    // Release knob
    initKnob(compReleaseKnob, "Compression Release", Params::ID_COMPRESSION_RELEASE, 10, 1000, 0.1, "ms");
    initLabel(compReleaseLabel, "RELEASE");
    compReleaseKnob->setSize(compactKnobSize, compactKnobSize);

    // Parameter attachments for compression
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_COMPRESSION_MIX, *compMixKnob));
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_COMPRESSION_THRESHOLD, *compThresholdKnob));
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_COMPRESSION_RATIO, *compRatioKnob));
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_COMPRESSION_ATTACK, *compAttackKnob));
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_COMPRESSION_RELEASE, *compReleaseKnob));

    // Pan section label
    panSectionLabel =
            std::unique_ptr<juce::Label>(createLabel("PAN", juce::Justification::centred));
    panSectionLabel->setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    panSectionLabel->setColour(juce::Label::textColourId,
                                 sectionColour.withAlpha(0.8f));
    addAndMakeVisible(panSectionLabel.get());

    // Pan knob
    initKnob(panKnob, "Pan Position", Params::ID_PAN, -100, 100, 0.1, "");
    panKnob->setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    panKnob->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    panKnob->setDoubleClickReturnValue(true, 0.0); // Center on double-click
    panKnob->setSize(compactKnobSize + 10, compactKnobSize + 10); // Slightly larger pan knob
    initLabel(panLabel, "PAN");

    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_PAN, *panKnob));

    // Flanger section label
    flangerSectionLabel =
            std::unique_ptr<juce::Label>(createLabel("FLANGER", juce::Justification::centred));
    flangerSectionLabel->setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    flangerSectionLabel->setColour(juce::Label::textColourId,
                                   sectionColour.withAlpha(0.8f));
    addAndMakeVisible(flangerSectionLabel.get());

    // Flanger knobs
    initKnob(flangerMixKnob, "Flanger Mix", Params::ID_FLANGER_MIX, 0, 100, 0.1, "");
    initLabel(flangerMixLabel, "MIX");
    flangerMixKnob->setSize(compactKnobSize, compactKnobSize);

    initKnob(flangerRateKnob, "Flanger Rate", Params::ID_FLANGER_RATE, 0.01, 20, 0.01, "Hz");
    initLabel(flangerRateLabel, "RATE");
    flangerRateKnob->setSize(compactKnobSize, compactKnobSize);

    initKnob(flangerDepthKnob, "Flanger Depth", Params::ID_FLANGER_DEPTH, 0, 100, 0.1, "");
    initLabel(flangerDepthLabel, "DEPTH");
    flangerDepthKnob->setSize(compactKnobSize, compactKnobSize);

    initKnob(flangerFeedbackKnob, "Flanger Feedback", Params::ID_FLANGER_FEEDBACK, 0, 100, 0.1, "");
    initLabel(flangerFeedbackLabel, "FDBK");
    flangerFeedbackKnob->setSize(compactKnobSize, compactKnobSize);

    // Parameter attachments for flanger
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_FLANGER_MIX, *flangerMixKnob));
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_FLANGER_RATE, *flangerRateKnob));
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_FLANGER_DEPTH, *flangerDepthKnob));
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_FLANGER_FEEDBACK, *flangerFeedbackKnob));

    // Phaser section label
    phaserSectionLabel =
            std::unique_ptr<juce::Label>(createLabel("PHASER", juce::Justification::centred));
    phaserSectionLabel->setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    phaserSectionLabel->setColour(juce::Label::textColourId,
                                  sectionColour.withAlpha(0.8f));
    addAndMakeVisible(phaserSectionLabel.get());

    // Phaser knobs
    initKnob(phaserMixKnob, "Phaser Mix", Params::ID_PHASER_MIX, 0, 100, 0.1, "");
    initLabel(phaserMixLabel, "MIX");
    phaserMixKnob->setSize(compactKnobSize, compactKnobSize);

    initKnob(phaserRateKnob, "Phaser Rate", Params::ID_PHASER_RATE, 0.01, 20, 0.01, "Hz");
    initLabel(phaserRateLabel, "RATE");
    phaserRateKnob->setSize(compactKnobSize, compactKnobSize);

    initKnob(phaserDepthKnob, "Phaser Depth", Params::ID_PHASER_DEPTH, 0, 100, 0.1, "");
    initLabel(phaserDepthLabel, "DEPTH");
    phaserDepthKnob->setSize(compactKnobSize, compactKnobSize);

    initKnob(phaserFeedbackKnob, "Phaser Feedback", Params::ID_PHASER_FEEDBACK, 0, 100, 0.1, "");
    initLabel(phaserFeedbackLabel, "FDBK");
    phaserFeedbackKnob->setSize(compactKnobSize, compactKnobSize);

    initKnob(phaserStagesKnob, "Phaser Stages", Params::ID_PHASER_STAGES, 2, 12, 2, "");
    initLabel(phaserStagesLabel, "STAGES");
    phaserStagesKnob->setSize(compactKnobSize, compactKnobSize);

    // Parameter attachments for phaser
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_PHASER_MIX, *phaserMixKnob));
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_PHASER_RATE, *phaserRateKnob));
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_PHASER_DEPTH, *phaserDepthKnob));
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_PHASER_FEEDBACK, *phaserFeedbackKnob));
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), Params::ID_PHASER_STAGES, *phaserStagesKnob));
}

void EffectsSection::updateDelayRateKnobTooltip() {
    if (!delayRateKnob || !delayBpmSyncToggle)
        return;

    bool isBpmSync = false;

    // First try to get the value from the parameter directly, which is safer during initialization
    auto *param = processor.getAPVTS().getParameter(Params::ID_DELAY_BPM_SYNC);
    if (param)
        isBpmSync = param->getValue() > 0.5f;
    else if (delayBpmSyncToggle) // Fallback to button state if available
        isBpmSync = delayBpmSyncToggle->getValue();

    float value = delayRateKnob->getValue();

    if (isBpmSync) {
        // For BPM sync mode, show musical note values
        juce::String noteValue;
        if (value < 10.0f)
            noteValue = "Whole note";
        else if (value < 30.0f)
            noteValue = "Half note";
        else if (value < 50.0f)
            noteValue = "Quarter note";
        else if (value < 70.0f)
            noteValue = "Eighth note";
        else if (value < 90.0f)
            noteValue = "Sixteenth note";
        else
            noteValue = "Thirty-second note";

        delayRateKnob->setTooltip("Delay Rate (BPM Sync): " + noteValue);
    } else {
        // For milliseconds mode, show millisecond value
        float delayTimeMs = juce::jmap(value, 0.0f, 100.0f, 10.0f, 1000.0f);
        delayRateKnob->setTooltip("Delay Rate: " + juce::String(static_cast<int>(delayTimeMs)) + " ms");
    }
}

void EffectsSection::updatePingPongTooltip() {
    if (!delayPingPongToggle)
        return;

    bool isPingPong = delayPingPongToggle->getValue();

    if (isPingPong)
        delayPingPongToggle->setTooltip("Ping Pong Delay: ON - Echoes alternate between left and right channels");
    else
        delayPingPongToggle->setTooltip("Ping Pong Delay: OFF - Standard stereo delay");
}

void EffectsSection::updateBpmSyncTooltip() {
    if (!delayBpmSyncToggle)
        return;

    bool isBpmSync = delayBpmSyncToggle->getValue();

    if (isBpmSync)
        delayBpmSyncToggle->setTooltip("BPM Sync: ON - Delay time synced to musical note values");
    else
        delayBpmSyncToggle->setTooltip("BPM Sync: OFF - Delay time in milliseconds (10-1000ms)");
}

EffectsSection::~EffectsSection() {
    clearAttachments();
}

void EffectsSection::paint(juce::Graphics &g) {
    BaseSectionComponent::paint(g);

    auto area = getLocalBounds();
    const int numSectionsPerRow = 3;
    const float sectionWidth = area.getWidth() / static_cast<float>(numSectionsPerRow);

    // Calculate Y positions based on resized()
    const int knobSize = 36;
    const int labelHeight = 18;
    const int titleHeight = 20;
    const int verticalPadding = 10;
    const int internalPadding = 5;
    const int row1TitleY = firstRowY - titleHeight - internalPadding; // Title above knobs
    const int row1KnobY = firstRowY;
    const int row1LabelY = row1KnobY + knobSize + internalPadding;
    const int rowHeight = titleHeight + knobSize + labelHeight + verticalPadding + internalPadding * 2;
    const int row2TitleY = row1TitleY + rowHeight;
    const int row2KnobY = row1KnobY + rowHeight;
    const int row2LabelY = row1LabelY + rowHeight;

    g.setColour(sectionColour.withAlpha(0.3f));

    // --- Row 1 Dividers ---
    const int divider1X = static_cast<int>(sectionWidth);
    const int divider2X = static_cast<int>(sectionWidth * 2);
    // Draw dividers for row 1 (between stutter/reverb and reverb/delay)
    g.drawLine(divider1X, row1TitleY + 5, divider1X, row1LabelY + labelHeight - 5, 1.0f);
    g.drawLine(divider2X, row1TitleY + 5, divider2X, row1LabelY + labelHeight - 5, 1.0f);

    // --- Row 2 Dividers ---
    // Dividers between Compression/Pan and Pan/Flanger (assuming this order in row 2)
    g.drawLine(divider1X, row2TitleY + 5, divider1X, row2LabelY + labelHeight - 5, 1.0f);
    g.drawLine(divider2X, row2TitleY + 5, divider2X, row2LabelY + labelHeight - 5, 1.0f);
    // If more effects are added later, add more dividers here
}

void EffectsSection::resized() {
    auto area = getLocalBounds();

    const int numSectionsPerRow = 3;
    const float sectionWidth = area.getWidth() / static_cast<float>(numSectionsPerRow);
    const int knobSize = 36;
    const int labelHeight = 18;
    const int titleHeight = 20;
    const int verticalPadding = 10; // Space between rows
    const int internalPadding = 5; // Space within a section (title->knob, knob->label)

    // Calculate Y positions
    const int row1TitleY = firstRowY - titleHeight - internalPadding;
    const int row1KnobY = firstRowY;
    const int row1LabelY = row1KnobY + knobSize + internalPadding;

    const int rowHeight = titleHeight + knobSize + labelHeight + verticalPadding + internalPadding * 2; // Total height of one row section

    const int row2TitleY = row1TitleY + rowHeight;
    const int row2KnobY = row1KnobY + rowHeight;
    const int row2LabelY = row1LabelY + rowHeight;

    // Divider X positions (same for both rows relative to start of row)
    const int divider1X = static_cast<int>(sectionWidth);
    const int divider2X = static_cast<int>(sectionWidth * 2);

    // --- Row 1 Positioning (Stutter, Reverb, Delay) ---
    // Section titles
    stutterSectionLabel->setBounds(0, row1TitleY, sectionWidth, titleHeight);
    reverbSectionLabel->setBounds(divider1X, row1TitleY, sectionWidth, titleHeight);
    delaySectionLabel->setBounds(divider2X, row1TitleY, sectionWidth, titleHeight);

    // Stutter
    const int stutterCenterX = static_cast<int>(sectionWidth * 0.5f);
    stutterKnob->setBounds(stutterCenterX - knobSize / 2, row1KnobY, knobSize, knobSize);
    stutterLabel->setBounds(stutterCenterX - knobSize / 2, row1LabelY, knobSize, labelHeight);

    // Reverb
    const int reverbKnobCount = 3;
    const float reverbKnobGap = sectionWidth / (reverbKnobCount + 1);
    float currentX = divider1X + reverbKnobGap;
    reverbMixKnob->setBounds(currentX - knobSize / 2, row1KnobY, knobSize, knobSize);
    reverbMixLabel->setBounds(currentX - knobSize / 2, row1LabelY, knobSize, labelHeight);
    currentX += reverbKnobGap;
    reverbTimeKnob->setBounds(currentX - knobSize / 2, row1KnobY, knobSize, knobSize);
    reverbTimeLabel->setBounds(currentX - knobSize / 2, row1LabelY, knobSize, labelHeight);
    currentX += reverbKnobGap;
    reverbWidthKnob->setBounds(currentX - knobSize / 2, row1KnobY, knobSize, knobSize);
    reverbWidthLabel->setBounds(currentX - knobSize / 2, row1LabelY, knobSize, labelHeight);

    // Delay
    const int delayKnobCount = 3;
    const float delayKnobGap = sectionWidth / (delayKnobCount + 1);
    currentX = divider2X + delayKnobGap;
    delayMixKnob->setBounds(currentX - knobSize / 2, row1KnobY, knobSize, knobSize);
    delayMixLabel->setBounds(currentX - knobSize / 2, row1LabelY, knobSize, labelHeight);
    currentX += delayKnobGap;
    delayRateKnob->setBounds(currentX - knobSize / 2, row1KnobY, knobSize, knobSize);
    delayRateLabel->setBounds(currentX - knobSize / 2, row1LabelY, knobSize, labelHeight);
    currentX += delayKnobGap;
    delayFeedbackKnob->setBounds(currentX - knobSize / 2, row1KnobY, knobSize, knobSize);
    delayFeedbackLabel->setBounds(currentX - knobSize / 2, row1LabelY, knobSize, labelHeight);

    // Delay Toggles (Position relative to Delay section title)
    const int toggleWidth = 28;
    const int toggleHeight = 16;
    delayPingPongToggle->setBounds(divider2X + 5, row1TitleY + (titleHeight-toggleHeight)/2, toggleWidth, toggleHeight);
    delayBpmSyncToggle->setBounds(divider2X + sectionWidth - toggleWidth - 5, row1TitleY + (titleHeight-toggleHeight)/2, toggleWidth, toggleHeight);

    // --- Row 2 Positioning (Compression, Pan) ---
    // Section titles
    compSectionLabel->setBounds(0, row2TitleY, sectionWidth, titleHeight);
    panSectionLabel->setBounds(divider1X, row2TitleY, sectionWidth, titleHeight);
    flangerSectionLabel->setBounds(divider2X, row2TitleY, sectionWidth, titleHeight);

    // Compression
    const int compKnobCount = 5;
    const float compKnobGap = sectionWidth / (compKnobCount + 1);
    currentX = 0 + compKnobGap; // Start from X=0 for the second row
    compMixKnob->setBounds(currentX - knobSize / 2, row2KnobY, knobSize, knobSize);
    compMixLabel->setBounds(currentX - knobSize / 2, row2LabelY, knobSize, labelHeight);
    currentX += compKnobGap;
    compThresholdKnob->setBounds(currentX - knobSize / 2, row2KnobY, knobSize, knobSize);
    compThresholdLabel->setBounds(currentX - knobSize / 2, row2LabelY, knobSize, labelHeight);
    currentX += compKnobGap;
    compRatioKnob->setBounds(currentX - knobSize / 2, row2KnobY, knobSize, knobSize);
    compRatioLabel->setBounds(currentX - knobSize / 2, row2LabelY, knobSize, labelHeight);
    currentX += compKnobGap;
    compAttackKnob->setBounds(currentX - knobSize / 2, row2KnobY, knobSize, knobSize);
    compAttackLabel->setBounds(currentX - knobSize / 2, row2LabelY, knobSize, labelHeight);
    currentX += compKnobGap;
    compReleaseKnob->setBounds(currentX - knobSize / 2, row2KnobY, knobSize, knobSize);
    compReleaseLabel->setBounds(currentX - knobSize / 2, row2LabelY, knobSize, labelHeight);

    // Pan
    const int panCenterX = divider1X + static_cast<int>(sectionWidth * 0.5f);
    const int panKnobSize = knobSize + 10; // Match the slightly larger size set in init
    panKnob->setBounds(panCenterX - panKnobSize / 2, row2KnobY - 5, panKnobSize, panKnobSize);
    // panLabel is redundant

    // Flanger (assumes it's the third section in row 2)
    const int flangerKnobCount = 4;
    const float flangerKnobGap = sectionWidth / (flangerKnobCount + 1);
    currentX = divider2X + flangerKnobGap;
    
    flangerMixKnob->setBounds(currentX - knobSize / 2, row2KnobY, knobSize, knobSize);
    flangerMixLabel->setBounds(currentX - knobSize / 2, row2LabelY, knobSize, labelHeight);
    
    currentX += flangerKnobGap;
    flangerRateKnob->setBounds(currentX - knobSize / 2, row2KnobY, knobSize, knobSize);
    flangerRateLabel->setBounds(currentX - knobSize / 2, row2LabelY, knobSize, labelHeight);
    
    currentX += flangerKnobGap;
    flangerDepthKnob->setBounds(currentX - knobSize / 2, row2KnobY, knobSize, knobSize);
    flangerDepthLabel->setBounds(currentX - knobSize / 2, row2LabelY, knobSize, labelHeight);
    
    currentX += flangerKnobGap;
    flangerFeedbackKnob->setBounds(currentX - knobSize / 2, row2KnobY, knobSize, knobSize);
    flangerFeedbackLabel->setBounds(currentX - knobSize / 2, row2LabelY, knobSize, labelHeight);
    
    // Add a third row for Phaser
    const int row3TitleY = row2TitleY + rowHeight;
    const int row3KnobY = row2KnobY + rowHeight;
    const int row3LabelY = row2LabelY + rowHeight;
    
    // Phaser section title (position in first third only)
    phaserSectionLabel->setBounds(0, row3TitleY, sectionWidth, titleHeight);
    
    // Phaser knobs - use sectionWidth instead of full width to make it one-third like others
    const int phaserKnobCount = 5;
    const float phaserKnobGap = sectionWidth / (phaserKnobCount + 1); // Changed to use sectionWidth
    
    currentX = phaserKnobGap;
    phaserMixKnob->setBounds(currentX - knobSize / 2, row3KnobY, knobSize, knobSize);
    phaserMixLabel->setBounds(currentX - knobSize / 2, row3LabelY, knobSize, labelHeight);
    
    currentX += phaserKnobGap;
    phaserRateKnob->setBounds(currentX - knobSize / 2, row3KnobY, knobSize, knobSize);
    phaserRateLabel->setBounds(currentX - knobSize / 2, row3LabelY, knobSize, labelHeight);
    
    currentX += phaserKnobGap;
    phaserDepthKnob->setBounds(currentX - knobSize / 2, row3KnobY, knobSize, knobSize);
    phaserDepthLabel->setBounds(currentX - knobSize / 2, row3LabelY, knobSize, labelHeight);
    
    currentX += phaserKnobGap;
    phaserFeedbackKnob->setBounds(currentX - knobSize / 2, row3KnobY, knobSize, knobSize);
    phaserFeedbackLabel->setBounds(currentX - knobSize / 2, row3LabelY, knobSize, labelHeight);
    
    currentX += phaserKnobGap;
    phaserStagesKnob->setBounds(currentX - knobSize / 2, row3KnobY, knobSize, knobSize);
    phaserStagesLabel->setBounds(currentX - knobSize / 2, row3LabelY, knobSize, labelHeight);
}