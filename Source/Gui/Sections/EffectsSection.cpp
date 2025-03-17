//
// Created by Shaked Melman on 01/03/2025.
//

#include "EffectsSection.h"

EffectsSection::EffectsSection(PluginEditor &e, PluginProcessor &p)
        : BaseSectionComponent(e, p, "EFFECTS", juce::Colour(0xffd9a652)) {
    // Smaller size for compact UI
    const int compactKnobSize = 36;

    // Stutter effect control
    initKnob(stutterKnob, "Stutter", AppState::ID_STUTTER_PROBABILITY, 0, 100, 0.1, "");
    initLabel(stutterLabel, "STUTTER");
    stutterKnob->setSize(compactKnobSize, compactKnobSize);

    // Stutter section label
    stutterSectionLabel =
            std::unique_ptr<juce::Label>(createLabel("STUTTER", juce::Justification::centred));
    stutterSectionLabel->setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    stutterSectionLabel->setColour(juce::Label::textColourId,
                                   sectionColour.withAlpha(0.8f));
    addAndMakeVisible(stutterSectionLabel.get());

    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), AppState::ID_STUTTER_PROBABILITY, *stutterKnob));

    // Reverb section label
    reverbSectionLabel =
            std::unique_ptr<juce::Label>(createLabel("REVERB", juce::Justification::centred));
    reverbSectionLabel->setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    reverbSectionLabel->setColour(juce::Label::textColourId,
                                  sectionColour.withAlpha(0.8f));
    addAndMakeVisible(reverbSectionLabel.get());

    // Reverb controls - compact UI without value text
    // Mix knob
    initKnob(reverbMixKnob, "Reverb Mix", AppState::ID_REVERB_MIX, 0, 100, 0.1, "");
    initLabel(reverbMixLabel, "MIX");
    reverbMixKnob->setSize(compactKnobSize, compactKnobSize);

    // Probability knob
    initKnob(reverbProbabilityKnob,
             "Reverb Probability",
             AppState::ID_REVERB_PROBABILITY,
             0,
             100,
             0.1,
             "");
    initLabel(reverbProbabilityLabel, "PROB");
    reverbProbabilityKnob->setSize(compactKnobSize, compactKnobSize);

    // Time knob
    initKnob(reverbTimeKnob, "Reverb Time", AppState::ID_REVERB_TIME, 0, 100, 0.1, "");
    initLabel(reverbTimeLabel, "TIME");
    reverbTimeKnob->setSize(compactKnobSize, compactKnobSize);

    // Width knob
    initKnob(reverbWidthKnob, "Reverb Width", AppState::ID_REVERB_WIDTH, 0, 100, 0.1, "");
    initLabel(reverbWidthLabel, "WIDTH");
    reverbWidthKnob->setSize(compactKnobSize, compactKnobSize);

    // Parameter attachments for reverb
    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), AppState::ID_REVERB_MIX, *reverbMixKnob));

    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), AppState::ID_REVERB_PROBABILITY, *reverbProbabilityKnob));

    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), AppState::ID_REVERB_TIME, *reverbTimeKnob));

    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), AppState::ID_REVERB_WIDTH, *reverbWidthKnob));

    // Delay section label
    delaySectionLabel =
            std::unique_ptr<juce::Label>(createLabel("DELAY", juce::Justification::centred));
    delaySectionLabel->setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    delaySectionLabel->setColour(juce::Label::textColourId,
                                 sectionColour.withAlpha(0.8f));
    addAndMakeVisible(delaySectionLabel.get());

    // Delay controls - compact UI without value text
    // Mix knob
    initKnob(delayMixKnob, "Delay Mix", AppState::ID_DELAY_MIX, 0, 100, 0.1, "");
    initLabel(delayMixLabel, "MIX");
    delayMixKnob->setSize(compactKnobSize, compactKnobSize);

    // Probability knob
    initKnob(delayProbabilityKnob, "Delay Probability", AppState::ID_DELAY_PROBABILITY, 0, 100, 0.1, "");
    initLabel(delayProbabilityLabel, "PROB");
    delayProbabilityKnob->setSize(compactKnobSize, compactKnobSize);

    // Rate knob - set initial tooltip based on BPM sync state
    initKnob(delayRateKnob, "Delay Rate", AppState::ID_DELAY_RATE, 0, 100, 0.1, "");
    initLabel(delayRateLabel, "RATE");
    delayRateKnob->setSize(compactKnobSize, compactKnobSize);

    // Feedback knob
    initKnob(delayFeedbackKnob, "Delay Feedback", AppState::ID_DELAY_FEEDBACK, 0, 100, 0.1, "");
    initLabel(delayFeedbackLabel, "FDBK");
    delayFeedbackKnob->setSize(compactKnobSize, compactKnobSize);

    // Add value listener to update tooltip when rate changes
    delayRateKnob->onValueChange = [this]() {
        updateDelayRateKnobTooltip();
    };

    // Create the toggle components first, before accessing their state
    // Ping Pong toggle
    delayPingPongToggle = std::make_unique<Toggle>(sectionColour);
    delayPingPongToggle->setTooltip("Toggle between ping pong mode and normal delay");
    delayPingPongToggle->setSize(28, 16);

    // Get the parameter directly
    auto *pingPongParam = dynamic_cast<juce::AudioParameterBool *>(
            processor.getAPVTS().getParameter(AppState::ID_DELAY_PING_PONG));

    // Set initial value from parameter
    if (pingPongParam)
        delayPingPongToggle->setValue(pingPongParam->get());

    // Direct connection to parameter
    delayPingPongToggle->onValueChanged = [this](bool newValue) {
        auto *param = processor.getAPVTS().getParameter(AppState::ID_DELAY_PING_PONG);
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
            processor.getAPVTS().getParameter(AppState::ID_DELAY_BPM_SYNC));

    // Set initial value from parameter
    if (bpmSyncParam)
        delayBpmSyncToggle->setValue(bpmSyncParam->get());

    // Direct connection to parameter
    delayBpmSyncToggle->onValueChanged = [this](bool newValue) {
        auto *param = processor.getAPVTS().getParameter(AppState::ID_DELAY_BPM_SYNC);
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
                    processor.getAPVTS(), AppState::ID_DELAY_MIX, *delayMixKnob));

    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), AppState::ID_DELAY_PROBABILITY, *delayProbabilityKnob));

    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), AppState::ID_DELAY_RATE, *delayRateKnob));

    sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getAPVTS(), AppState::ID_DELAY_FEEDBACK, *delayFeedbackKnob));
}

void EffectsSection::updateDelayRateKnobTooltip() {
    if (!delayRateKnob || !delayBpmSyncToggle)
        return;

    bool isBpmSync = false;

    // First try to get the value from the parameter directly, which is safer during initialization
    auto *param = processor.getAPVTS().getParameter(AppState::ID_DELAY_BPM_SYNC);
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
    // Call the base class paint method
    BaseSectionComponent::paint(g);

    auto area = getLocalBounds();

    const int knobSize = 36;
    const int rowY = firstRowY + knobSize / 2;

    // Section divisions
    const int totalSections = 3; // Stutter, Reverb, Delay
    const float sectionWidth = area.getWidth() / static_cast<float>(totalSections);

    // First vertical line - after stutter, before reverb
    const int divider1X = static_cast<int>(sectionWidth);
    g.setColour(sectionColour.withAlpha(0.3f));
    g.drawLine(divider1X, rowY - 30 + 5, divider1X, rowY + 60, 1.0f);

    // Second vertical line - after reverb section, before delay
    const int divider2X = static_cast<int>(sectionWidth * 2);
    g.setColour(sectionColour.withAlpha(0.3f));
    g.drawLine(divider2X, rowY - 30 + 5, divider2X, rowY + 60, 1.0f);
}

void EffectsSection::resized() {
    auto area = getLocalBounds();

    // More compact knob size for tighter UI
    const int knobSize = 36;
    const int labelHeight = 18;
    const int rowY = firstRowY + 20;

    // Section divisions
    const int totalSections = 3; // Stutter, Reverb, Delay
    const float sectionWidth = area.getWidth() / static_cast<float>(totalSections);

    // Divider positions
    const int divider1X = static_cast<int>(sectionWidth);
    const int divider2X = static_cast<int>(sectionWidth * 2);

    // Section centers
    const int stutterCenterX = static_cast<int>(sectionWidth * 0.5f);
    const int reverbCenterX = static_cast<int>(sectionWidth * 1.5f);
    const int delayCenterX = static_cast<int>(sectionWidth * 2.5f);

    // Section titles positions
    stutterSectionLabel->setBounds(0, rowY - 25, divider1X, 20);
    reverbSectionLabel->setBounds(divider1X, rowY - 25, sectionWidth, 20);
    delaySectionLabel->setBounds(divider2X, rowY - 25, sectionWidth, 20);

    // Position stutter knob in center of its section
    stutterKnob->setBounds(stutterCenterX - knobSize / 2, rowY, knobSize, knobSize);
    stutterLabel->setBounds(stutterCenterX - knobSize / 2, rowY + knobSize, knobSize, labelHeight);

    // Position reverb knobs with even spacing
    const int reverbKnobCount = 4;
    const float reverbKnobGap = sectionWidth / (reverbKnobCount + 1);

    float currentX = divider1X + reverbKnobGap;
    reverbMixKnob->setBounds(currentX - knobSize / 2, rowY, knobSize, knobSize);
    reverbMixLabel->setBounds(currentX - knobSize / 2, rowY + knobSize, knobSize, labelHeight);

    currentX += reverbKnobGap;
    reverbTimeKnob->setBounds(currentX - knobSize / 2, rowY, knobSize, knobSize);
    reverbTimeLabel->setBounds(currentX - knobSize / 2, rowY + knobSize, knobSize, labelHeight);

    currentX += reverbKnobGap;
    reverbProbabilityKnob->setBounds(currentX - knobSize / 2, rowY, knobSize, knobSize);
    reverbProbabilityLabel->setBounds(currentX - knobSize / 2, rowY + knobSize, knobSize, labelHeight);

    currentX += reverbKnobGap;
    reverbWidthKnob->setBounds(currentX - knobSize / 2, rowY, knobSize, knobSize);
    reverbWidthLabel->setBounds(currentX - knobSize / 2, rowY + knobSize, knobSize, labelHeight);

    // Position toggle components
    const int toggleWidth = 28;
    const int toggleHeight = 16;
    const int toggleY = rowY - 25;

    // Position Ping Pong toggle in the top left of delay section
    delayPingPongToggle->setBounds(divider2X + 5, toggleY, toggleWidth, toggleHeight);

    // Position BPM sync toggle at the top right
    delayBpmSyncToggle->setBounds(divider2X + sectionWidth - toggleWidth - 5, toggleY, toggleWidth, toggleHeight);

    // Position delay knobs with even spacing
    const int delayKnobCount = 4; // 4 knobs (mix, rate, probability, feedback)
    const float delayKnobGap = sectionWidth / (delayKnobCount + 1);

    // Position delay knobs
    currentX = divider2X + delayKnobGap;
    delayMixKnob->setBounds(currentX - knobSize / 2, rowY, knobSize, knobSize);
    delayMixLabel->setBounds(currentX - knobSize / 2, rowY + knobSize, knobSize, labelHeight);

    currentX += delayKnobGap;
    delayRateKnob->setBounds(currentX - knobSize / 2, rowY, knobSize, knobSize);
    delayRateLabel->setBounds(currentX - knobSize / 2, rowY + knobSize, knobSize, labelHeight);

    currentX += delayKnobGap;
    delayProbabilityKnob->setBounds(currentX - knobSize / 2, rowY, knobSize, knobSize);
    delayProbabilityLabel->setBounds(currentX - knobSize / 2, rowY + knobSize, knobSize, labelHeight);

    currentX += delayKnobGap;
    delayFeedbackKnob->setBounds(currentX - knobSize / 2, rowY, knobSize, knobSize);
    delayFeedbackLabel->setBounds(currentX - knobSize / 2, rowY + knobSize, knobSize, labelHeight);
}