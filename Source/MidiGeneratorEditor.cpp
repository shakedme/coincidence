#include <memory>

#include "MidiGeneratorProcessor.h"

//==============================================================================
MidiGeneratorEditor::MidiGeneratorEditor(MidiGeneratorProcessor& p)
    : AudioProcessorEditor(&p)
    , processor(p)
{
    // Set custom look and feel
    setLookAndFeel(&customLookAndFeel);

    // Set up main sections
    setupGrooveSection();
    setupPitchSection();
    setupGlitchSection();
    setupSampleSection();

    // Set up keyboard
    setupKeyboard();

    // Set editor size
    setSize(800, 800);

    startTimerHz(30);

    updateRateLabelsForRhythmMode();
}

MidiGeneratorEditor::~MidiGeneratorEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

//==============================================================================
void MidiGeneratorEditor::paint(juce::Graphics& g)
{
    // Fill background with dark grey
    g.fillAll(juce::Colour(0xff222222));

    // Draw title banner with a metallic gradient
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff505050), 0, 0, juce::Colour(0xff303030), 0, 60, false));
    g.fillRect(0, 0, getWidth(), 60);

    // Add subtle highlight to give a metal panel effect
    g.setColour(juce::Colour(0x20ffffff));
    g.fillRect(0, 3, getWidth(), 2);

    // Draw plugin logo/title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(34.0f, juce::Font::bold));
    g.drawText(
        "Jammer", getLocalBounds().removeFromTop(60), juce::Justification::centred, true);

    // Draw version
    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
    g.drawText("v1.0",
               getLocalBounds().removeFromTop(60).removeFromRight(60),
               juce::Justification::centredRight,
               true);

    // Draw section panels with metallic effect
    auto drawMetallicPanel =
        [&](juce::Rectangle<int> bounds, juce::Colour baseColor, juce::Colour accentColor)
    {
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

        // Draw a subtle accent line under the section header
        g.setColour(accentColor.withAlpha(0.5f));
        g.drawLine(bounds.getX() + 10,
                   bounds.getY() + 35,
                   bounds.getRight() - 10,
                   bounds.getY() + 35,
                   1.0f);

        // Draw screws in the corners
        auto* lf = dynamic_cast<MidiGeneratorLookAndFeel*>(&getLookAndFeel());
        if (lf != nullptr)
        {
            lf->drawScrew(g, bounds.getX() + 10, bounds.getY() + 10, 8);
            lf->drawScrew(g, bounds.getRight() - 10, bounds.getY() + 10, 8);
            lf->drawScrew(g, bounds.getX() + 10, bounds.getBottom() - 10, 8);
            lf->drawScrew(g, bounds.getRight() - 10, bounds.getBottom() - 10, 8);
        }
    };

    // Calculate main sections
    int sectionHeight = static_cast<int>((getHeight() - 80 - 80) * 0.65);
    int glitchHeight = static_cast<int>((getHeight() - 80 - 80) * 0.35) - 10;
    int sampleHeight =
        static_cast<int>((getHeight() - 80 - 80 - 100) * 0.30); // 30% of space

    int topSectionsBottom = 70 + sectionHeight;
    int glitchSectionBottom = topSectionsBottom + glitchHeight;

    // Create section areas for drawing
    auto grooveArea = juce::Rectangle<int>(10, 70, getWidth() / 2 - 20, sectionHeight);
    auto pitchArea =
        juce::Rectangle<int>(getWidth() / 2 + 10, 70, getWidth() / 2 - 20, sectionHeight);
    auto glitchArea =
        juce::Rectangle<int>(10, 70 + sectionHeight + 10, getWidth() - 20, glitchHeight);
    auto sampleArea =
        juce::Rectangle<int>(10, glitchSectionBottom, getWidth() - 20, sampleHeight - 10);

    // Draw the metallic panels for each section
    drawMetallicPanel(grooveArea,
                      juce::Colour(0xff2a2a2a),
                      juce::Colour(0xff52bfd9)); // Groove - blue accent
    drawMetallicPanel(pitchArea,
                      juce::Colour(0xff2a2a2a),
                      juce::Colour(0xff52d97d)); // Pitch - green accent
    drawMetallicPanel(glitchArea, juce::Colour(0xff2a2a2a), juce::Colour(0xffd9a652));

    // Draw a more prominent drop area
    g.setColour(isCurrentlyOver ? juce::Colours::white.withAlpha(0.4f)
                                : juce::Colours::white.withAlpha(0.1f));
    g.fillRect(sampleArea);

    g.setColour(isCurrentlyOver ? juce::Colours::white : juce::Colours::lightgrey);
    g.drawRect(sampleArea, isCurrentlyOver ? 3 : 1);

    // Enhanced text display
    g.setFont(16.0f);
    g.setColour(isCurrentlyOver ? juce::Colours::black
                                : juce::Colours::white.withAlpha(0.7f));
    g.drawText("Drop audio files here (.wav, .aif, .mp3, etc.)",
               sampleArea,
               juce::Justification::centred,
               true);

    // Enhanced sample drop area visualization
    if (isCurrentlyOver || processor.sampleList.empty())
    {
        auto sampleArea = juce::Rectangle<int>(
            10, glitchSectionBottom, getWidth() - 20, sampleHeight - 10);

        // Draw a more prominent drop area
        g.setColour(isCurrentlyOver ? juce::Colours::white.withAlpha(0.4f)
                                    : juce::Colours::white.withAlpha(0.1f));
        g.fillRect(sampleArea);

        g.setColour(isCurrentlyOver ? juce::Colours::white : juce::Colours::lightgrey);
        g.drawRect(sampleArea, isCurrentlyOver ? 3 : 1);

        // Enhanced text display
        g.setFont(16.0f);
        g.setColour(isCurrentlyOver ? juce::Colours::black
                                    : juce::Colours::white.withAlpha(0.7f));
        g.drawText(processor.sampleList.empty()
                       ? "Drag audio files here (.wav, .aif, .mp3, etc.)"
                       : "Drop audio files here to add to library",
                   sampleArea,
                   juce::Justification::centred,
                   true);

        // Add icon or visual cue
        if (isCurrentlyOver)
        {
            // Draw a simple arrow pointing down in the center
            int arrowWidth = 30;
            int arrowHeight = 30;
            int centerX = sampleArea.getCentreX();
            int topY = sampleArea.getCentreY() - 30;

            juce::Path arrow;
            arrow.startNewSubPath(centerX - arrowWidth / 2, topY);
            arrow.lineTo(centerX + arrowWidth / 2, topY);
            arrow.lineTo(centerX, topY + arrowHeight);
            arrow.closeSubPath();

            g.setColour(juce::Colours::black.withAlpha(0.7f));
            g.fillPath(arrow);
        }
    }
}

void MidiGeneratorEditor::resized()
{
    // Calculate main sections
    auto area = getLocalBounds();
    auto headerArea = area.removeFromTop(60);
    auto mainArea = area.removeFromTop(area.getHeight() - 80);

    // Calculate dimensions for the three sections
    // Keep the same proportions but make sure the Groove and Pitch sections don't grow too large
    int sectionHeight =
        static_cast<int>(mainArea.getHeight() * 0.45); // Reduced from 0.65 to 0.45
    int glitchHeight = mainArea.getHeight() * 0.25; // 25% of space for glitch
    int sampleHeight = mainArea.getHeight() * 0.30; // 30% of space for sample section

    int topSectionsBottom = 70 + sectionHeight;
    int glitchSectionBottom = topSectionsBottom + glitchHeight + 10;

    // Create section areas
    auto grooveArea = juce::Rectangle<int>(10, 70, getWidth() / 2 - 20, sectionHeight);
    auto pitchArea =
        juce::Rectangle<int>(getWidth() / 2 + 10, 70, getWidth() / 2 - 20, sectionHeight);
    auto glitchArea = juce::Rectangle<int>(
        10, 70 + sectionHeight + 10, getWidth() - 20, glitchHeight - 10);
    auto sampleArea =
        juce::Rectangle<int>(10, glitchSectionBottom, getWidth() - 20, sampleHeight - 10);

    // Position the section labels
    grooveSectionLabel->setBounds(grooveArea.getX(), 70, grooveArea.getWidth(), 30);
    pitchSectionLabel->setBounds(pitchArea.getX(), 70, pitchArea.getWidth(), 30);
    glitchSectionLabel->setBounds(
        glitchArea.getX(), topSectionsBottom, glitchArea.getWidth(), 30);
    sampleSectionLabel->setBounds(
        sampleArea.getX(), glitchSectionBottom, sampleArea.getWidth(), 30);

    // Configure the Groove section
    // Calculate even spacing for rate knobs
    const int knobSize = 60;
    const int labelHeight = 20;
    const int knobPadding =
        (grooveArea.getWidth() - (MidiGeneratorProcessor::NUM_RATE_OPTIONS * knobSize))
        / (MidiGeneratorProcessor::NUM_RATE_OPTIONS + 1);

    // Top row - Rate knobs
    const int rateKnobY = grooveArea.getY() + 45;
    for (int i = 0; i < MidiGeneratorProcessor::NUM_RATE_OPTIONS; ++i)
    {
        int xPos = grooveArea.getX() + knobPadding + i * (knobSize + knobPadding);
        rateKnobs[i]->setBounds(xPos, rateKnobY, knobSize, knobSize);
        rateLabels[i]->setBounds(xPos, rateKnobY + knobSize, knobSize, labelHeight);
    }

    // Middle row - Density (centered)
    const int middleRowY = rateKnobY + knobSize + labelHeight + 30;

    // Centered density knob
    densityKnob->setBounds(
        grooveArea.getCentreX() - knobSize / 2, middleRowY, knobSize, knobSize);
    densityLabel->setBounds(grooveArea.getCentreX() - knobSize / 2,
                            middleRowY + knobSize,
                            knobSize,
                            labelHeight);

    // Bottom row - Gate and Velocity controls
    const int bottomRowY = middleRowY + knobSize + labelHeight + 30;
    const int fourKnobPadding = (grooveArea.getWidth() - (4 * knobSize)) / 5;

    // Two gate knobs on the left
    gateKnob->setBounds(
        grooveArea.getX() + fourKnobPadding, bottomRowY, knobSize, knobSize);
    gateLabel->setBounds(grooveArea.getX() + fourKnobPadding,
                         bottomRowY + knobSize,
                         knobSize,
                         labelHeight);

    gateRandomKnob->setBounds(grooveArea.getX() + 2 * fourKnobPadding + knobSize,
                              bottomRowY,
                              knobSize,
                              knobSize);
    gateRandomLabel->setBounds(grooveArea.getX() + 2 * fourKnobPadding + knobSize,
                               bottomRowY + knobSize,
                               knobSize,
                               labelHeight);

    // Two velocity knobs on the right
    velocityKnob->setBounds(grooveArea.getX() + 3 * fourKnobPadding + 2 * knobSize,
                            bottomRowY,
                            knobSize,
                            knobSize);
    velocityLabel->setBounds(grooveArea.getX() + 3 * fourKnobPadding + 2 * knobSize,
                             bottomRowY + knobSize,
                             knobSize,
                             labelHeight);

    velocityRandomKnob->setBounds(grooveArea.getX() + 4 * fourKnobPadding + 3 * knobSize,
                                  bottomRowY,
                                  knobSize,
                                  knobSize);
    velocityRandomLabel->setBounds(grooveArea.getX() + 4 * fourKnobPadding + 3 * knobSize,
                                   bottomRowY + knobSize,
                                   knobSize,
                                   labelHeight);

    // Configure the Pitch section
    // Scale type dropdown at the top
    scaleTypeComboBox->setBounds(
        pitchArea.getCentreX() - 100, pitchArea.getY() + 45, 200, 30);
    scaleLabel->setBounds(pitchArea.getCentreX() - 50, pitchArea.getY() + 80, 100, 20);

    const int rhythmComboWidth = 100;
    const int rhythmComboHeight = 25; // Slightly smaller
    rhythmModeComboBox->setBounds(grooveArea.getCentreX() + knobSize / 2
                                      + 10, // Position it to the right of density
                                  middleRowY,
                                  rhythmComboWidth,
                                  rhythmComboHeight);
    rhythmModeLabel->setBounds(rhythmModeComboBox->getX(),
                               middleRowY + rhythmComboHeight,
                               rhythmComboWidth,
                               labelHeight);

    const int twoKnobPadding = (pitchArea.getWidth() - (2 * knobSize)) / 3;

    // Middle row - Semitone controls
    // Steps
    semitonesKnob->setBounds(
        pitchArea.getX() + twoKnobPadding, middleRowY, knobSize, knobSize);
    semitonesLabel->setBounds(
        pitchArea.getX() + twoKnobPadding, middleRowY + knobSize, knobSize, labelHeight);

    // Chance
    semitonesProbabilityKnob->setBounds(
        pitchArea.getX() + 2 * twoKnobPadding + knobSize, middleRowY, knobSize, knobSize);
    semitonesProbabilityLabel->setBounds(pitchArea.getX() + 2 * twoKnobPadding + knobSize,
                                         middleRowY + knobSize,
                                         knobSize,
                                         labelHeight);

    // Bottom row - Octave controls
    // Shift
    octavesKnob->setBounds(
        pitchArea.getX() + twoKnobPadding, bottomRowY, knobSize, knobSize);
    octavesLabel->setBounds(
        pitchArea.getX() + twoKnobPadding, bottomRowY + knobSize, knobSize, labelHeight);

    // Chance
    octavesProbabilityKnob->setBounds(
        pitchArea.getX() + 2 * twoKnobPadding + knobSize, bottomRowY, knobSize, knobSize);
    octavesProbabilityLabel->setBounds(pitchArea.getX() + 2 * twoKnobPadding + knobSize,
                                       bottomRowY + knobSize,
                                       knobSize,
                                       labelHeight);

    // Configure Glitch section - evenly distribute 6 knobs
    const int numGlitchKnobs = 6;
    const int glitchKnobPadding =
        (glitchArea.getWidth() - (numGlitchKnobs * knobSize)) / (numGlitchKnobs + 1);
    const int glitchKnobY = topSectionsBottom + 60;
    for (int i = 0; i < numGlitchKnobs; ++i)
    {
        int xPos =
            glitchArea.getX() + glitchKnobPadding + i * (knobSize + glitchKnobPadding);
        glitchKnobs[i]->setBounds(xPos, glitchKnobY, knobSize, knobSize);
        glitchLabels[i]->setBounds(xPos, glitchKnobY + knobSize, knobSize, labelHeight);
    }

    // Sample section layout
    int controlsY = glitchSectionBottom + 40; // Start below the section label

    // Sample list takes up left side
    int sampleListWidth = sampleArea.getWidth() * 0.6f;
    sampleListBox->setBounds(
        sampleArea.getX(), controlsY, sampleListWidth, sampleHeight - 80);

    // Buttons below the sample list
    int buttonY = controlsY + sampleHeight - 70;
    int buttonWidth = 80;
    addSampleButton->setBounds(sampleArea.getX(), buttonY, buttonWidth, 25);
    removeSampleButton->setBounds(
        sampleArea.getX() + buttonWidth + 5, buttonY, buttonWidth, 25);

    // Sample name display
    sampleNameLabel->setBounds(sampleArea.getX(), buttonY - 30, sampleListWidth, 25);

    // Right side controls
    int controlsX = sampleArea.getX() + sampleListWidth + 20;
    int controlsWidth = sampleArea.getWidth() - sampleListWidth - 30;

    randomizeToggle->setBounds(controlsX, controlsY, controlsWidth, 25);

    randomizeProbabilityLabel->setBounds(controlsX, controlsY + 40, 80, 25);
    randomizeProbabilitySlider->setBounds(
        controlsX + 90, controlsY + 40, controlsWidth - 100, 25);

    // Footer with keyboard - center it horizontally and move it below the sample section
    const int keyboardWidth = area.getWidth() - 20;
    const int keyboardHeight = 80;
    const int keyboardY = glitchSectionBottom + sampleHeight + 10;
    const int keyboardX = (getWidth() - keyboardWidth) / 2;

    keyboardComponent->setBounds(keyboardX, keyboardY, keyboardWidth, keyboardHeight);
}

//==============================================================================
juce::Label* MidiGeneratorEditor::createLabel(const juce::String& text,
                                              juce::Justification justification)
{
    auto* label = new juce::Label();
    label->setText(text, juce::dontSendNotification);
    label->setJustificationType(justification);
    label->setFont(juce::Font(14.0f));
    return label;
}

juce::Slider* MidiGeneratorEditor::createRotarySlider(const juce::String& tooltip)
{
    auto* slider =
        new juce::Slider(juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow);
    slider->setTooltip(tooltip);
    slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    slider->setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider->setColour(juce::Slider::textBoxBackgroundColourId,
                      juce::Colours::transparentBlack);
    slider->setColour(juce::Slider::textBoxOutlineColourId,
                      juce::Colours::transparentBlack);
    return slider;
}

//==============================================================================
void MidiGeneratorEditor::setupGrooveSection()
{
    // Create section header
    grooveSectionLabel =
        std::unique_ptr<juce::Label>(createLabel("GROOVE", juce::Justification::centred));
    grooveSectionLabel->setFont(juce::Font(20.0f, juce::Font::bold));
    grooveSectionLabel->setColour(juce::Label::textColourId,
                                  juce::Colour(0xff52bfd9)); // cyan/blue
    addAndMakeVisible(grooveSectionLabel.get());
    grooveSectionLabel->setBounds(10, 80, getWidth() / 2 - 20, 30);

    // Set up rate controls
    setupRateControls();

    setupRhythmModeControls();

    // Set up density control
    setupDensityControls();

    // Set up gate controls
    setupGateControls();

    // Set up velocity controls
    setupVelocityControls();
}

void MidiGeneratorEditor::setupPitchSection()
{
    // Create section header
    pitchSectionLabel =
        std::unique_ptr<juce::Label>(createLabel("PITCH", juce::Justification::centred));
    pitchSectionLabel->setFont(juce::Font(20.0f, juce::Font::bold));
    pitchSectionLabel->setColour(juce::Label::textColourId,
                                 juce::Colour(0xff52d97d)); // green
    addAndMakeVisible(pitchSectionLabel.get());
    pitchSectionLabel->setBounds(getWidth() / 2 + 10, 80, getWidth() / 2 - 20, 30);

    // Set up scale type controls
    setupScaleTypeControls();

    // Set up semitone controls
    setupSemitoneControls();

    // Set up octave controls
    setupOctaveControls();
}

void MidiGeneratorEditor::setupGlitchSection()
{
    // Create section header
    glitchSectionLabel =
        std::unique_ptr<juce::Label>(createLabel("GLITCH", juce::Justification::centred));
    glitchSectionLabel->setFont(juce::Font(20.0f, juce::Font::bold));
    glitchSectionLabel->setColour(juce::Label::textColourId,
                                  juce::Colour(0xffd9a652)); // amber
    //    addAndMakeVisible(glitchSectionLabel.get());
    // Position will be set in resized()

    // Create dummy knobs for the Glitch section
    const char* glitchNames[6] = {
        "CRUSH", "STUTTER", "CHAOS", "REVERSE", "JUMP", "GLIDE"};

    for (int i = 0; i < 6; ++i)
    {
        // Create glitch knob
        glitchKnobs[i] = std::unique_ptr<juce::Slider>(
            createRotarySlider(glitchNames[i] + juce::String(" intensity")));
        glitchKnobs[i]->setName("glitch_" + juce::String(i));
        glitchKnobs[i]->setRange(0.0, 100.0, 0.1);
        glitchKnobs[i]->setTextValueSuffix("%");
        //        addAndMakeVisible(glitchKnobs[i].get());

        // Create glitch label
        glitchLabels[i] = std::unique_ptr<juce::Label>(
            createLabel(glitchNames[i], juce::Justification::centred));
        glitchLabels[i]->setFont(juce::Font(14.0f, juce::Font::bold));
        //        addAndMakeVisible(glitchLabels[i].get());

        // These parameters don't exist yet, so we won't create attachments
    }
}

void MidiGeneratorEditor::setupKeyboard()
{
    // Create keyboard state
    keyboardState = std::make_unique<juce::MidiKeyboardState>();

    // Create keyboard component the traditional way
    keyboardComponent = std::make_unique<juce::MidiKeyboardComponent>(
        *keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard);

    keyboardComponent->setKeyWidth(16.0f);
    keyboardComponent->setAvailableRange(12, 96); // C2 to C7
    keyboardComponent->setLowestVisibleKey(48); // C3
    keyboardComponent->setOctaveForMiddleC(4);
    keyboardComponent->setColour(juce::MidiKeyboardComponent::shadowColourId,
                                 juce::Colours::transparentBlack);
    keyboardComponent->setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId,
                                 juce::Colour(0xff3a3a3a));
    keyboardComponent->setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,
                                 juce::Colour(0xff52bfd9));

    //    addAndMakeVisible(keyboardComponent.get());
}

// Method to update keyboard state with played notes
void MidiGeneratorEditor::updateKeyboardState(bool isNoteOn, int noteNumber, int velocity)
{
    if (keyboardState != nullptr)
    {
        if (isNoteOn)
            keyboardState->noteOn(1, noteNumber, static_cast<float>(velocity) / 127.0f);
        else
            keyboardState->noteOff(1, noteNumber, 0.0f);

        keyboardNeedsUpdate = true;
    }
}

void MidiGeneratorEditor::setupRhythmModeControls()
{
    // Create rhythm mode combo box
    rhythmModeComboBox = std::make_unique<juce::ComboBox>();
    rhythmModeComboBox->addItem("Normal", MidiGeneratorProcessor::RHYTHM_NORMAL + 1);
    rhythmModeComboBox->addItem("Dotted", MidiGeneratorProcessor::RHYTHM_DOTTED + 1);
    rhythmModeComboBox->addItem("Triplet", MidiGeneratorProcessor::RHYTHM_TRIPLET + 1);
    rhythmModeComboBox->setJustificationType(juce::Justification::centred);
    rhythmModeComboBox->setColour(juce::ComboBox::backgroundColourId,
                                  juce::Colour(0xff3a3a3a));
    rhythmModeComboBox->setColour(juce::ComboBox::textColourId, juce::Colours::white);
    rhythmModeComboBox->onChange = [this]() { updateRateLabelsForRhythmMode(); };
    addAndMakeVisible(rhythmModeComboBox.get());

    // Create rhythm mode label
    rhythmModeLabel =
        std::unique_ptr<juce::Label>(createLabel("MODE", juce::Justification::centred));
    rhythmModeLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(rhythmModeLabel.get());

    // Create parameter attachment
    comboBoxAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.parameters, "rhythm_mode", *rhythmModeComboBox));
}

void MidiGeneratorEditor::updateRateLabelsForRhythmMode()
{
    // Don't proceed if the combo box isn't initialized yet
    if (rhythmModeComboBox == nullptr)
        return;

    // Get the current rhythm mode from the processor
    auto rhythmMode = static_cast<MidiGeneratorProcessor::RhythmMode>(
        rhythmModeComboBox->getSelectedItemIndex());

    // Get the rhythm mode text suffix
    juce::String rhythmSuffix = processor.getRhythmModeText(rhythmMode);

    // Update the rate labels with the appropriate suffix
    const char* rateBaseNames[MidiGeneratorProcessor::NUM_RATE_OPTIONS] = {
        "1/2", "1/4", "1/8", "1/16", "1/32"};

    for (int i = 0; i < MidiGeneratorProcessor::NUM_RATE_OPTIONS; ++i)
    {
        // Check if the label exists
        if (rateLabels[i] != nullptr)
        {
            juce::String labelText = rateBaseNames[i];

            // Only append suffix if it's not empty (i.e., not NORMAL mode)
            if (rhythmSuffix.isNotEmpty())
                labelText += rhythmSuffix;

            rateLabels[i]->setText(labelText, juce::dontSendNotification);
        }
    }
}
//==============================================================================
void MidiGeneratorEditor::setupRateControls()
{
    // Rate labels - Base names without rhythm mode suffix for now
    const char* rateBaseNames[MidiGeneratorProcessor::NUM_RATE_OPTIONS] = {
        "1/2", "1/4", "1/8", "1/16", "1/32"};

    for (int i = 0; i < MidiGeneratorProcessor::NUM_RATE_OPTIONS; ++i)
    {
        // Create rate knob
        rateKnobs[i] = std::unique_ptr<juce::Slider>(
            createRotarySlider("Rate " + juce::String(rateBaseNames[i]) + " intensity"));
        rateKnobs[i]->setName("rate_" + juce::String(i));
        rateKnobs[i]->setRange(0.0, 100.0, 0.1);
        rateKnobs[i]->setTextValueSuffix("%");
        addAndMakeVisible(rateKnobs[i].get());

        // Create rate label
        rateLabels[i] = std::unique_ptr<juce::Label>(
            createLabel(rateBaseNames[i], juce::Justification::centred));
        rateLabels[i]->setFont(juce::Font(16.0f, juce::Font::bold));
        addAndMakeVisible(rateLabels[i].get());

        // Create parameter attachment
        sliderAttachments.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.parameters,
                "rate_" + juce::String(i) + "_value",
                *rateKnobs[i]));
    }

    // Update the rate labels for the current rhythm mode
    updateRateLabelsForRhythmMode();
}
void MidiGeneratorEditor::setupDensityControls()
{
    // Create density knob
    densityKnob =
        std::unique_ptr<juce::Slider>(createRotarySlider("Overall density/probability"));
    densityKnob->setName("density");
    densityKnob->setRange(0.0, 100.0, 0.1);
    densityKnob->setTextValueSuffix("%");
    addAndMakeVisible(densityKnob.get());

    // Create density label
    densityLabel = std::unique_ptr<juce::Label>(
        createLabel("DENSITY", juce::Justification::centred));
    densityLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(densityLabel.get());

    // Create parameter attachment
    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "density", *densityKnob));
}

void MidiGeneratorEditor::setupGateControls()
{
    // Create gate knob
    gateKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Gate length"));
    gateKnob->setName("gate");
    gateKnob->setRange(0.0, 100.0, 0.1);
    gateKnob->setTextValueSuffix("%");
    addAndMakeVisible(gateKnob.get());

    // Create gate label
    gateLabel =
        std::unique_ptr<juce::Label>(createLabel("GATE", juce::Justification::centred));
    gateLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(gateLabel.get());

    // Create gate random knob
    gateRandomKnob =
        std::unique_ptr<juce::Slider>(createRotarySlider("Gate randomization"));
    gateRandomKnob->setName("gate_random");
    gateRandomKnob->setRange(0.0, 100.0, 0.1);
    gateRandomKnob->setTextValueSuffix("%");
    addAndMakeVisible(gateRandomKnob.get());

    // Create gate random label
    gateRandomLabel =
        std::unique_ptr<juce::Label>(createLabel("RNDM", juce::Justification::centred));
    gateRandomLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(gateRandomLabel.get());

    // Create parameter attachments
    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "gate", *gateKnob));
    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "gate_randomize", *gateRandomKnob));
}

void MidiGeneratorEditor::setupVelocityControls()
{
    // Create velocity knob
    velocityKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Velocity"));
    velocityKnob->setName("velocity");
    velocityKnob->setRange(0.0, 100.0, 0.1);
    velocityKnob->setTextValueSuffix("%");
    addAndMakeVisible(velocityKnob.get());

    // Create velocity label
    velocityLabel =
        std::unique_ptr<juce::Label>(createLabel("VELO", juce::Justification::centred));
    velocityLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(velocityLabel.get());

    // Create velocity random knob
    velocityRandomKnob =
        std::unique_ptr<juce::Slider>(createRotarySlider("Velocity randomization"));
    velocityRandomKnob->setName("velocity_random");
    velocityRandomKnob->setRange(0.0, 100.0, 0.1);
    velocityRandomKnob->setTextValueSuffix("%");
    addAndMakeVisible(velocityRandomKnob.get());

    // Create velocity random label
    velocityRandomLabel =
        std::unique_ptr<juce::Label>(createLabel("RNDM", juce::Justification::centred));
    velocityRandomLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(velocityRandomLabel.get());

    // Create parameter attachments
    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "velocity", *velocityKnob));
    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "velocity_randomize", *velocityRandomKnob));
}

void MidiGeneratorEditor::setupScaleTypeControls()
{
    // Create scale type combo box
    scaleTypeComboBox = std::make_unique<juce::ComboBox>();
    scaleTypeComboBox->addItem("MAJOR", MidiGeneratorProcessor::SCALE_MAJOR + 1);
    scaleTypeComboBox->addItem("MINOR", MidiGeneratorProcessor::SCALE_MINOR + 1);
    scaleTypeComboBox->addItem("PENTATONIC",
                               MidiGeneratorProcessor::SCALE_PENTATONIC + 1);
    scaleTypeComboBox->setJustificationType(juce::Justification::centred);
    scaleTypeComboBox->setColour(juce::ComboBox::backgroundColourId,
                                 juce::Colour(0xff3a3a3a));
    scaleTypeComboBox->setColour(juce::ComboBox::textColourId, juce::Colours::white);
    addAndMakeVisible(scaleTypeComboBox.get());

    // Create scale label
    scaleLabel =
        std::unique_ptr<juce::Label>(createLabel("SCALE", juce::Justification::centred));
    scaleLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(scaleLabel.get());

    // Create parameter attachment
    comboBoxAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.parameters, "scale_type", *scaleTypeComboBox));
}

void MidiGeneratorEditor::setupSemitoneControls()
{
    // Create semitones knob
    semitonesKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Semitone range"));
    semitonesKnob->setName("semitones");
    semitonesKnob->setRange(0, 12, 1);
    addAndMakeVisible(semitonesKnob.get());

    // Create semitones label
    semitonesLabel =
        std::unique_ptr<juce::Label>(createLabel("STEPS", juce::Justification::centred));
    semitonesLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(semitonesLabel.get());

    // Create semitones probability knob
    semitonesProbabilityKnob = std::unique_ptr<juce::Slider>(
        createRotarySlider("Semitone variation probability"));
    semitonesProbabilityKnob->setName("semitones_prob");
    semitonesProbabilityKnob->setRange(0.0, 100.0, 0.1);
    semitonesProbabilityKnob->setTextValueSuffix("%");
    addAndMakeVisible(semitonesProbabilityKnob.get());

    // Create semitones probability label
    semitonesProbabilityLabel =
        std::unique_ptr<juce::Label>(createLabel("CHANCE", juce::Justification::centred));
    semitonesProbabilityLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(semitonesProbabilityLabel.get());

    // Create parameter attachments
    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "semitones", *semitonesKnob));
    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "semitones_prob", *semitonesProbabilityKnob));
}

void MidiGeneratorEditor::setupOctaveControls()
{
    // Create octaves knob
    octavesKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Octave range"));
    octavesKnob->setName("octaves");
    octavesKnob->setRange(0, 3, 1);
    addAndMakeVisible(octavesKnob.get());

    // Create octaves label
    octavesLabel =
        std::unique_ptr<juce::Label>(createLabel("SHIFT", juce::Justification::centred));
    octavesLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(octavesLabel.get());

    // Create octaves probability knob
    octavesProbabilityKnob =
        std::unique_ptr<juce::Slider>(createRotarySlider("Octave variation probability"));
    octavesProbabilityKnob->setName("octaves_prob");
    octavesProbabilityKnob->setRange(0.0, 100.0, 0.1);
    octavesProbabilityKnob->setTextValueSuffix("%");
    addAndMakeVisible(octavesProbabilityKnob.get());

    // Create octaves probability label
    octavesProbabilityLabel =
        std::unique_ptr<juce::Label>(createLabel("CHANCE", juce::Justification::centred));
    octavesProbabilityLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(octavesProbabilityLabel.get());

    // Create parameter attachments
    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "octaves", *octavesKnob));
    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "octaves_prob", *octavesProbabilityKnob));
}

void MidiGeneratorEditor::repaintRandomizationControls()
{
    // Repaint knobs with randomization
    if (gateKnob != nullptr)
        gateKnob->repaint();
    if (velocityKnob != nullptr)
        velocityKnob->repaint();
}

void MidiGeneratorEditor::timerCallback()
{
    // Regularly update the UI elements that need to reflect processor state
    repaintRandomizationControls();

    if (rhythmModeComboBox != nullptr)
        updateRateLabelsForRhythmMode();

    if (keyboardNeedsUpdate && keyboardComponent != nullptr)
    {
        keyboardComponent->repaint();
        keyboardNeedsUpdate = false;
    }

    // Update keyboard state to handle any stray notes
    // (this can help correct any missed note-off events)
    if (isShowing() && keyboardComponent != nullptr && !processor.isNoteActive()
        && keyboardState != nullptr)
    {
        // Check if any notes appear to be on in the keyboard that shouldn't be
        keyboardState->allNotesOff(1);
        keyboardComponent->repaint();
    }
}

void MidiGeneratorEditor::updateActiveSample(int index)
{
    currentlyPlayingSampleIndex = index;
}

void MidiGeneratorEditor::setupSampleSection()
{
    // Create section header
    sampleSectionLabel =
        std::unique_ptr<juce::Label>(createLabel("SAMPLE", juce::Justification::centred));
    sampleSectionLabel->setFont(juce::Font(20.0f, juce::Font::bold));
    sampleSectionLabel->setColour(juce::Label::textColourId,
                                  juce::Colour(0xffbf52d9)); // Purple
    addAndMakeVisible(sampleSectionLabel.get());

    // Create sample list model and table
    sampleModel = std::make_unique<SampleListModel>(processor);

    sampleListBox =
        std::make_unique<juce::TableListBox>("Sample List", sampleModel.get());
    sampleListBox->setHeaderHeight(22);
    sampleListBox->getHeader().addColumn("Sample", 1, 200);
    sampleListBox->setMultipleSelectionEnabled(false);
    addAndMakeVisible(sampleListBox.get());

    // Sample management buttons
    addSampleButton = std::make_unique<juce::TextButton>("Add");
    addSampleButton->onClick = [this]()
    {
        juce::FileChooser chooser(
            "Select a sample to load...",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.wav;*.aif;*.aiff");

        chooser.launchAsync(juce::FileBrowserComponent::openMode,
                            [this](const juce::FileChooser& chooser)
                            {
                                if (chooser.getResult().existsAsFile())
                                {
                                    auto file = chooser.getResult();
                                    processor.addSample(file);
                                    sampleListBox->updateContent();
                                }
                            });
    };

    addAndMakeVisible(addSampleButton.get());

    removeSampleButton = std::make_unique<juce::TextButton>("Remove");
    removeSampleButton->onClick = [this]()
    {
        int selectedRow = sampleListBox->getSelectedRow();
        if (selectedRow >= 0)
        {
            processor.removeSample(selectedRow);
            sampleListBox->updateContent();
        }
    };
    addAndMakeVisible(removeSampleButton.get());

    // Randomization controls
    randomizeToggle = std::make_unique<juce::ToggleButton>("Randomize Samples");
    randomizeToggle->setToggleState(false, juce::dontSendNotification);
    addAndMakeVisible(randomizeToggle.get());

    randomizeProbabilitySlider = std::make_unique<juce::Slider>(
        juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
    randomizeProbabilitySlider->setRange(0.0, 100.0, 1.0);
    randomizeProbabilitySlider->setValue(100.0);
    randomizeProbabilitySlider->setTextValueSuffix("%");
    addAndMakeVisible(randomizeProbabilitySlider.get());

    randomizeProbabilityLabel = std::unique_ptr<juce::Label>(
        createLabel("Probability", juce::Justification::centredLeft));
    addAndMakeVisible(randomizeProbabilityLabel.get());

    sampleNameLabel = std::unique_ptr<juce::Label>(
        createLabel("No samples loaded", juce::Justification::centred));
    sampleNameLabel->setFont(juce::Font(14.0f));
    addAndMakeVisible(sampleNameLabel.get());

    // Add parameter attachments for new controls
    buttonAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.parameters, "randomize_samples", *randomizeToggle));

    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "randomize_probability", *randomizeProbabilitySlider));
}

int MidiGeneratorEditor::SampleListModel::getNumRows()
{
    return processor.getNumSamples();
}

void MidiGeneratorEditor::SampleListModel::paintRowBackground(
    juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colour(0x80bf52d9)); // Purple highlight
    else if (rowNumber % 2)
        g.fillAll(juce::Colour(0xff3a3a3a)); // Darker
    else
        g.fillAll(juce::Colour(0xff444444)); // Lighter
}

void MidiGeneratorEditor::SampleListModel::paintCell(juce::Graphics& g,
                                                     int rowNumber,
                                                     int columnId,
                                                     int width,
                                                     int height,
                                                     bool rowIsSelected)
{
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);

    if (rowNumber < processor.getNumSamples())
    {
        if (columnId == 1) // Sample name
        {
            g.drawText(processor.getSampleName(rowNumber),
                       2,
                       0,
                       width - 4,
                       height,
                       juce::Justification::centredLeft);
        }
    }
}

void MidiGeneratorEditor::SampleListModel::cellClicked(int rowNumber,
                                                       int columnId,
                                                       const juce::MouseEvent&)
{
    if (rowNumber < processor.getNumSamples())
    {
        if (columnId == 1) // Sample name - select this sample
        {
            processor.selectSample(rowNumber);
        }
    }
}

void MidiGeneratorEditor::fileDragEnter(const juce::StringArray& files, int x, int y)
{
    if (isInterestedInFileDrag(files))
    {
        // Print debug info
        DBG("File drag entered: " + files[0]);
        isCurrentlyOver = true;
        repaint();
    }
}

void MidiGeneratorEditor::fileDragExit(const juce::StringArray& files)
{
    isCurrentlyOver = false;
    repaint();
}

bool MidiGeneratorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    // Check if any of the files are audio files - expanded file extension list
    for (int i = 0; i < files.size(); ++i)
    {
        juce::File f(files[i]);
        if (f.hasFileExtension("wav;aif;aiff;mp3;flac;ogg;m4a;wma"))
            return true;
    }
    return false;
}

void MidiGeneratorEditor::filesDropped(const juce::StringArray& files, int x, int y)
{
    // First, reset the drag state
    isCurrentlyOver = false;

    // Check if the drop is in the sample section area
    auto bounds = getBounds();
    int sectionHeight = static_cast<int>((getHeight() - 80 - 80 - 100) * 0.45);
    int glitchHeight = static_cast<int>((getHeight() - 80 - 80 - 100) * 0.25);

    int topSectionsBottom = 70 + sectionHeight;
    int glitchSectionBottom = topSectionsBottom + glitchHeight;
    int sampleSectionY = glitchSectionBottom;

    // Calculate sample section area
    auto sampleArea =
        juce::Rectangle<int>(10,
                             sampleSectionY,
                             getWidth(),
                             static_cast<int>((getHeight() - 80 - 80 - 100) * 0.30));

    // Only process drops in the sample section
    //    if (sampleArea.contains(x, y))
    //    {
    // Process the dropped files
    bool needsUpdate = false;

    for (const auto& i: files)
    {
        juce::File file(i);
        if (file.existsAsFile() && file.hasFileExtension("wav;aif;aiff;mp3"))
        {
            DBG("Loading dropped sample: " + file.getFullPathName());
            processor.addSample(file);
            needsUpdate = true;
        }
    }

    if (needsUpdate)
    {
        sampleListBox->updateContent();
        repaint();
    }
    if (processor.sampleList.empty())
    {
        sampleNameLabel->setText("No samples loaded", juce::dontSendNotification);
    }
    else
    {
        sampleNameLabel->setText(
            "",
            juce::dontSendNotification);
    }
}