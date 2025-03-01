#include <memory>

#include "MidiGeneratorProcessor.h"

//==============================================================================
MidiGeneratorEditor::MidiGeneratorEditor(MidiGeneratorProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    // Set custom look and feel
    setLookAndFeel(&customLookAndFeel);

    // Set up main sections
    setupGrooveSection();
    setupPitchSection();
    setupGlitchSection();

    // Set up keyboard
    setupKeyboard();

    // Set editor size
    setSize(800, 700);

    startTimerHz(30);
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
    g.fillAll(juce::Colour(0xff2a2a2a));

    // Draw title banner
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3a3a3a), 0, 0,
                                           juce::Colour(0xff2a2a2a), 0, 40, false));
    g.fillRect(0, 0, getWidth(), 60);

    // Draw plugin logo/title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(34.0f, juce::Font::bold));
    g.drawText("Jammer", getLocalBounds().removeFromTop(60), juce::Justification::centred, true);

    // Draw version
    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
    g.drawText("v1.0", getLocalBounds().removeFromTop(60).removeFromRight(60), juce::Justification::centredRight, true);

    // Draw section borders
    g.setColour(juce::Colour(0xff3a3a3a));

    // Groove section border
    g.drawRect(10, 70, getWidth()/2 - 20, (getHeight() - 80 - 80) * 0.7, 1);

    // Pitch section border
    g.drawRect(getWidth()/2 + 10, 70, getWidth()/2 - 20, (getHeight() - 80 - 80) * 0.7, 1);

    // Glitch section border
    g.drawRect(10, 70 + (getHeight() - 80 - 80) * 0.7 + 10, getWidth() - 20, (getHeight() - 80 - 80) * 0.3 - 10, 1);
}

void MidiGeneratorEditor::resized()
{
    // Calculate main sections
    auto area = getLocalBounds();
    auto headerArea = area.removeFromTop(60);
    auto mainArea = area.removeFromTop(area.getHeight() - 80);

    // Calculate dimensions for the three sections
    int sectionHeight = static_cast<int>(mainArea.getHeight() * 0.7);
    int glitchHeight = mainArea.getHeight() - sectionHeight;

    // Create section areas
    auto grooveArea = juce::Rectangle<int>(10, 70, getWidth()/2 - 20, sectionHeight);
    auto pitchArea = juce::Rectangle<int>(getWidth()/2 + 10, 70, getWidth()/2 - 20, sectionHeight);
    auto glitchArea = juce::Rectangle<int>(10, 70 + sectionHeight + 10, getWidth() - 20, glitchHeight - 10);

    // Position the section labels
    grooveSectionLabel->setBounds(grooveArea.getX(), 70, grooveArea.getWidth(), 30);
    pitchSectionLabel->setBounds(pitchArea.getX(), 70, pitchArea.getWidth(), 30);
    glitchSectionLabel->setBounds(glitchArea.getX(), glitchArea.getY(), glitchArea.getWidth(), 30);

    // Configure the Groove section
    // Calculate even spacing for rate knobs
    const int knobSize = 60;
    const int labelHeight = 20;
    const int knobPadding = (grooveArea.getWidth() - (MidiGeneratorProcessor::NUM_RATE_OPTIONS * knobSize)) /
                            (MidiGeneratorProcessor::NUM_RATE_OPTIONS + 1);

    // Top row - Rate knobs
    const int rateKnobY = grooveArea.getY() + 40;
    for (int i = 0; i < MidiGeneratorProcessor::NUM_RATE_OPTIONS; ++i)
    {
        int xPos = grooveArea.getX() + knobPadding + i * (knobSize + knobPadding);
        rateKnobs[i]->setBounds(xPos, rateKnobY, knobSize, knobSize);
        rateLabels[i]->setBounds(xPos, rateKnobY + knobSize, knobSize, labelHeight);
    }

    // Middle row - Density (centered)
    const int middleRowY = rateKnobY + knobSize + labelHeight + 30;

    // Centered density knob
    densityKnob->setBounds(grooveArea.getCentreX() - knobSize/2, middleRowY, knobSize, knobSize);
    densityLabel->setBounds(grooveArea.getCentreX() - knobSize/2, middleRowY + knobSize, knobSize, labelHeight);

    // Bottom row - Gate and Velocity controls
    const int bottomRowY = middleRowY + knobSize + labelHeight + 30;
    const int fourKnobPadding = (grooveArea.getWidth() - (4 * knobSize)) / 5;

    // Two gate knobs on the left
    gateKnob->setBounds(grooveArea.getX() + fourKnobPadding, bottomRowY, knobSize, knobSize);
    gateLabel->setBounds(grooveArea.getX() + fourKnobPadding, bottomRowY + knobSize, knobSize, labelHeight);

    gateRandomKnob->setBounds(grooveArea.getX() + 2 * fourKnobPadding + knobSize, bottomRowY, knobSize, knobSize);
    gateRandomLabel->setBounds(grooveArea.getX() + 2 * fourKnobPadding + knobSize, bottomRowY + knobSize, knobSize, labelHeight);

    // Two velocity knobs on the right
    velocityKnob->setBounds(grooveArea.getX() + 3 * fourKnobPadding + 2 * knobSize, bottomRowY, knobSize, knobSize);
    velocityLabel->setBounds(grooveArea.getX() + 3 * fourKnobPadding + 2 * knobSize, bottomRowY + knobSize, knobSize, labelHeight);

    velocityRandomKnob->setBounds(grooveArea.getX() + 4 * fourKnobPadding + 3 * knobSize, bottomRowY, knobSize, knobSize);
    velocityRandomLabel->setBounds(grooveArea.getX() + 4 * fourKnobPadding + 3 * knobSize, bottomRowY + knobSize, knobSize, labelHeight);

    // Configure the Pitch section
    // Scale type dropdown at the top
    scaleTypeComboBox->setBounds(pitchArea.getCentreX() - 100, pitchArea.getY() + 40, 200, 30);
    scaleLabel->setBounds(pitchArea.getCentreX() - 50, pitchArea.getY() + 75, 100, 20);

    const int twoKnobPadding = (pitchArea.getWidth() - (2 * knobSize)) / 3;

    // Middle row - Semitone controls
    // Steps
    semitonesKnob->setBounds(pitchArea.getX() + twoKnobPadding, middleRowY, knobSize, knobSize);
    semitonesLabel->setBounds(pitchArea.getX() + twoKnobPadding, middleRowY + knobSize, knobSize, labelHeight);

    // Chance
    semitonesProbabilityKnob->setBounds(pitchArea.getX() + 2 * twoKnobPadding + knobSize, middleRowY, knobSize, knobSize);
    semitonesProbabilityLabel->setBounds(pitchArea.getX() + 2 * twoKnobPadding + knobSize, middleRowY + knobSize, knobSize, labelHeight);

    // Bottom row - Octave controls
    // Shift
    octavesKnob->setBounds(pitchArea.getX() + twoKnobPadding, bottomRowY, knobSize, knobSize);
    octavesLabel->setBounds(pitchArea.getX() + twoKnobPadding, bottomRowY + knobSize, knobSize, labelHeight);

    // Chance
    octavesProbabilityKnob->setBounds(pitchArea.getX() + 2 * twoKnobPadding + knobSize, bottomRowY, knobSize, knobSize);
    octavesProbabilityLabel->setBounds(pitchArea.getX() + 2 * twoKnobPadding + knobSize, bottomRowY + knobSize, knobSize, labelHeight);

    // Configure Glitch section - evenly distribute 6 knobs
    const int numGlitchKnobs = 6;
    const int glitchKnobPadding = (glitchArea.getWidth() - (numGlitchKnobs * knobSize)) / (numGlitchKnobs + 1);
    const int glitchKnobY = glitchArea.getY() + 40;

    for (int i = 0; i < numGlitchKnobs; ++i)
    {
        int xPos = glitchArea.getX() + glitchKnobPadding + i * (knobSize + glitchKnobPadding);
        glitchKnobs[i]->setBounds(xPos, glitchKnobY, knobSize, knobSize);
        glitchLabels[i]->setBounds(xPos, glitchKnobY + knobSize, knobSize, labelHeight);
    }

    // Footer with keyboard - center it horizontally
    const int keyboardWidth = area.getWidth() - 20;
    const int keyboardHeight = area.getHeight() - 10;
    const int keyboardX = (getWidth() - keyboardWidth) / 2;

    keyboardComponent->setBounds(keyboardX, area.getY(), keyboardWidth, keyboardHeight);
}

//==============================================================================
juce::Label* MidiGeneratorEditor::createLabel(const juce::String& text, juce::Justification justification)
{
    auto* label = new juce::Label();
    label->setText(text, juce::dontSendNotification);
    label->setJustificationType(justification);
    label->setFont(juce::Font(14.0f));
    return label;
}

juce::Slider* MidiGeneratorEditor::createRotarySlider(const juce::String& tooltip)
{
    auto* slider = new juce::Slider(juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow);
    slider->setTooltip(tooltip);
    slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    slider->setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    return slider;
}

//==============================================================================
void MidiGeneratorEditor::setupGrooveSection()
{
    // Create section header
    grooveSectionLabel = std::unique_ptr<juce::Label>(createLabel("GROOVE", juce::Justification::centred));
    grooveSectionLabel->setFont(juce::Font(20.0f, juce::Font::bold));
    grooveSectionLabel->setColour(juce::Label::textColourId, juce::Colour(0xff52bfd9)); // cyan/blue
    addAndMakeVisible(grooveSectionLabel.get());
    grooveSectionLabel->setBounds(10, 80, getWidth()/2 - 20, 30);

    // Set up rate controls
    setupRateControls();

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
    pitchSectionLabel = std::unique_ptr<juce::Label>(createLabel("PITCH", juce::Justification::centred));
    pitchSectionLabel->setFont(juce::Font(20.0f, juce::Font::bold));
    pitchSectionLabel->setColour(juce::Label::textColourId, juce::Colour(0xff52d97d)); // green
    addAndMakeVisible(pitchSectionLabel.get());
    pitchSectionLabel->setBounds(getWidth()/2 + 10, 80, getWidth()/2 - 20, 30);

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
    glitchSectionLabel = std::unique_ptr<juce::Label>(createLabel("GLITCH", juce::Justification::centred));
    glitchSectionLabel->setFont(juce::Font(20.0f, juce::Font::bold));
    glitchSectionLabel->setColour(juce::Label::textColourId, juce::Colour(0xffd9a652)); // amber
    addAndMakeVisible(glitchSectionLabel.get());
    // Position will be set in resized()

    // Create dummy knobs for the Glitch section
    const char* glitchNames[6] = { "CRUSH", "STUTTER", "CHAOS", "REVERSE", "JUMP", "GLIDE" };

    for (int i = 0; i < 6; ++i)
    {
        // Create glitch knob
        glitchKnobs[i] = std::unique_ptr<juce::Slider>(createRotarySlider(glitchNames[i] + juce::String(" intensity")));
        glitchKnobs[i]->setName("glitch_" + juce::String(i));
        glitchKnobs[i]->setRange(0.0, 100.0, 0.1);
        glitchKnobs[i]->setTextValueSuffix("%");
        addAndMakeVisible(glitchKnobs[i].get());

        // Create glitch label
        glitchLabels[i] = std::unique_ptr<juce::Label>(createLabel(glitchNames[i], juce::Justification::centred));
        glitchLabels[i]->setFont(juce::Font(14.0f, juce::Font::bold));
        addAndMakeVisible(glitchLabels[i].get());

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
    keyboardComponent->setLowestVisibleKey(48);   // C3
    keyboardComponent->setOctaveForMiddleC(4);
    keyboardComponent->setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    keyboardComponent->setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xff3a3a3a));
    keyboardComponent->setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0xff52bfd9));

    addAndMakeVisible(keyboardComponent.get());
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

        keyboardComponent->repaint();
    }
}

//==============================================================================
void MidiGeneratorEditor::setupRateControls()
{
    // Rate labels - Update with 1/2 and adjust positions
    const char* rateNames[MidiGeneratorProcessor::NUM_RATE_OPTIONS] = { "1/2", "1/4", "1/8", "1/16", "1/32" };

    for (int i = 0; i < MidiGeneratorProcessor::NUM_RATE_OPTIONS; ++i)
    {
        // Create rate knob
        rateKnobs[i] = std::unique_ptr<juce::Slider>(createRotarySlider("Rate " + juce::String(rateNames[i]) + " intensity"));
        rateKnobs[i]->setName("rate_" + juce::String(i));
        rateKnobs[i]->setRange(0.0, 100.0, 0.1);
        rateKnobs[i]->setTextValueSuffix("%");
        addAndMakeVisible(rateKnobs[i].get());

        // Create rate label
        rateLabels[i] = std::unique_ptr<juce::Label>(createLabel(rateNames[i], juce::Justification::centred));
        rateLabels[i]->setFont(juce::Font(16.0f, juce::Font::bold));
        addAndMakeVisible(rateLabels[i].get());

        // Create parameter attachment
        sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "rate_" + juce::String(i) + "_value", *rateKnobs[i]));
    }
}

void MidiGeneratorEditor::setupDensityControls()
{
    // Create density knob
    densityKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Overall density/probability"));
    densityKnob->setName("density");
    densityKnob->setRange(0.0, 100.0, 0.1);
    densityKnob->setTextValueSuffix("%");
    addAndMakeVisible(densityKnob.get());

    // Create density label
    densityLabel = std::unique_ptr<juce::Label>(createLabel("DENSITY", juce::Justification::centred));
    densityLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(densityLabel.get());

    // Create parameter attachment
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
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
    gateLabel = std::unique_ptr<juce::Label>(createLabel("GATE", juce::Justification::centred));
    gateLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(gateLabel.get());

    // Create gate random knob
    gateRandomKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Gate randomization"));
    gateRandomKnob->setName("gate_random");
    gateRandomKnob->setRange(0.0, 100.0, 0.1);
    gateRandomKnob->setTextValueSuffix("%");
    addAndMakeVisible(gateRandomKnob.get());

    // Create gate random label
    gateRandomLabel = std::unique_ptr<juce::Label>(createLabel("RNDM", juce::Justification::centred));
    gateRandomLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(gateRandomLabel.get());

    // Create parameter attachments
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "gate", *gateKnob));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
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
    velocityLabel = std::unique_ptr<juce::Label>(createLabel("VELO", juce::Justification::centred));
    velocityLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(velocityLabel.get());

    // Create velocity random knob
    velocityRandomKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Velocity randomization"));
    velocityRandomKnob->setName("velocity_random");
    velocityRandomKnob->setRange(0.0, 100.0, 0.1);
    velocityRandomKnob->setTextValueSuffix("%");
    addAndMakeVisible(velocityRandomKnob.get());

    // Create velocity random label
    velocityRandomLabel = std::unique_ptr<juce::Label>(createLabel("RNDM", juce::Justification::centred));
    velocityRandomLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(velocityRandomLabel.get());

    // Create parameter attachments
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "velocity", *velocityKnob));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "velocity_randomize", *velocityRandomKnob));
}

void MidiGeneratorEditor::setupScaleTypeControls()
{
    // Create scale type combo box
    scaleTypeComboBox = std::make_unique<juce::ComboBox>();
    scaleTypeComboBox->addItem("MAJOR", MidiGeneratorProcessor::SCALE_MAJOR + 1);
    scaleTypeComboBox->addItem("MINOR", MidiGeneratorProcessor::SCALE_MINOR + 1);
    scaleTypeComboBox->addItem("PENTATONIC", MidiGeneratorProcessor::SCALE_PENTATONIC + 1);
    scaleTypeComboBox->setJustificationType(juce::Justification::centred);
    scaleTypeComboBox->setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff3a3a3a));
    scaleTypeComboBox->setColour(juce::ComboBox::textColourId, juce::Colours::white);
    addAndMakeVisible(scaleTypeComboBox.get());

    // Create scale label
    scaleLabel = std::unique_ptr<juce::Label>(createLabel("TYPE", juce::Justification::centred));
    scaleLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(scaleLabel.get());

    // Create parameter attachment
    comboBoxAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
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
    semitonesLabel = std::unique_ptr<juce::Label>(createLabel("STEPS", juce::Justification::centred));
    semitonesLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(semitonesLabel.get());

    // Create semitones probability knob
    semitonesProbabilityKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Semitone variation probability"));
    semitonesProbabilityKnob->setName("semitones_prob");
    semitonesProbabilityKnob->setRange(0.0, 100.0, 0.1);
    semitonesProbabilityKnob->setTextValueSuffix("%");
    addAndMakeVisible(semitonesProbabilityKnob.get());

    // Create semitones probability label
    semitonesProbabilityLabel = std::unique_ptr<juce::Label>(createLabel("CHANCE", juce::Justification::centred));
    semitonesProbabilityLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(semitonesProbabilityLabel.get());

    // Create parameter attachments
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "semitones", *semitonesKnob));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
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
    octavesLabel = std::unique_ptr<juce::Label>(createLabel("SHIFT", juce::Justification::centred));
    octavesLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(octavesLabel.get());

    // Create octaves probability knob
    octavesProbabilityKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Octave variation probability"));
    octavesProbabilityKnob->setName("octaves_prob");
    octavesProbabilityKnob->setRange(0.0, 100.0, 0.1);
    octavesProbabilityKnob->setTextValueSuffix("%");
    addAndMakeVisible(octavesProbabilityKnob.get());

    // Create octaves probability label
    octavesProbabilityLabel = std::unique_ptr<juce::Label>(createLabel("CHANCE", juce::Justification::centred));
    octavesProbabilityLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(octavesProbabilityLabel.get());

    // Create parameter attachments
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "octaves", *octavesKnob));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
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