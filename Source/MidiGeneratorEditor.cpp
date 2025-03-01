#include "MidiGeneratorProcessor.h"

//==============================================================================
MidiGeneratorEditor::MidiGeneratorEditor(MidiGeneratorProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    // Set custom look and feel
    setLookAndFeel(&customLookAndFeel);

    // Set up tabbed component
    setupTabbedComponent();

    // Set up rhythm tab
    setupRhythmTab();

    // Set up melody tab
    setupMelodyTab();

    // Set up keyboard
    setupKeyboard();

    // Set editor size
    setSize(800, 600);

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
}

void MidiGeneratorEditor::resized()
{
    // Logo area
    auto area = getLocalBounds();
    auto logoArea = area.removeFromTop(60);

    // Main area with tabs
    auto mainArea = area.removeFromTop(area.getHeight() - 80);

    // Footer with keyboard
    auto keyboardArea = area;

    // Set tabbed component bounds
    tabbedComponent->setBounds(mainArea);

    // Set keyboard bounds - make it fill the entire width
    keyboardComponent->setBounds(keyboardArea.reduced(5, 5));
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
void MidiGeneratorEditor::setupTabbedComponent()
{
    tabbedComponent = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);

    // Create rhythm tab with custom color
    rhythmTab = std::make_unique<juce::Component>();
    tabbedComponent->addTab("RHYTHM", juce::Colour(0xff2a2a2a), rhythmTab.get(), false);

    // Create melody tab with custom color
    melodyTab = std::make_unique<juce::Component>();
    tabbedComponent->addTab("MELODY", juce::Colour(0xff2a2a2a), melodyTab.get(), false);

    // Style the tabs
    tabbedComponent->setOutline(0);
    tabbedComponent->setTabBarDepth(32);

    addAndMakeVisible(tabbedComponent.get());
}

void MidiGeneratorEditor::setupRhythmTab()
{
    // Set up rate controls
    setupRateControls();

    // Set up density control
    setupDensityControls();

    // Set up gate controls
    setupGateControls();

    // Set up velocity controls
    setupVelocityControls();
}

void MidiGeneratorEditor::setupMelodyTab()
{
    // Set up scale type controls
    setupScaleTypeControls();

    // Set up shifter controls
    setupShifterControls();

    // Set up semitone controls
    setupSemitoneControls();

    // Set up octave controls
    setupOctaveControls();
}

void MidiGeneratorEditor::setupKeyboard()
{
    // Create keyboard state
    keyboardState = std::make_unique<juce::MidiKeyboardState>();

    // Create keyboard component the traditional way
    keyboardComponent = std::unique_ptr<juce::MidiKeyboardComponent>(
        new juce::MidiKeyboardComponent(*keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard));

    keyboardComponent->setKeyWidth(16.0f);
    keyboardComponent->setAvailableRange(36, 96); // C2 to C7
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
    // Section header
    auto* header = createLabel("GROOVE", juce::Justification::centred);
    header->setFont(juce::Font(20.0f, juce::Font::bold));
    header->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    header->setBounds(240, 20, 200, 30);
    rhythmTab->addAndMakeVisible(header);

    // Rate labels - Update with 1/2 and adjust positions
    const char* rateNames[MidiGeneratorProcessor::NUM_RATE_OPTIONS] = { "1/2", "1/4", "1/8", "1/16", "1/32" };
    const int rateX[MidiGeneratorProcessor::NUM_RATE_OPTIONS] = { 110, 230, 350, 470, 590 };

    for (int i = 0; i < MidiGeneratorProcessor::NUM_RATE_OPTIONS; ++i)
    {
        // Create rate knob
        rateKnobs[i] = std::unique_ptr<juce::Slider>(createRotarySlider("Rate " + juce::String(rateNames[i]) + " intensity"));
        rateKnobs[i]->setName("rate_" + juce::String(i));
        rateKnobs[i]->setRange(0.0, 100.0, 0.1);
        rateKnobs[i]->setTextValueSuffix("%");
        rhythmTab->addAndMakeVisible(rateKnobs[i].get());
        rateKnobs[i]->setBounds(rateX[i], 60, 80, 100);

        // Create rate label
        rateLabels[i] = std::unique_ptr<juce::Label>(createLabel(rateNames[i], juce::Justification::centred));
        rateLabels[i]->setFont(juce::Font(16.0f, juce::Font::bold));
        rhythmTab->addAndMakeVisible(rateLabels[i].get());
        rateLabels[i]->setBounds(rateX[i], 150, 80, 30);

        // Create parameter attachment
        sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "rate_" + juce::String(i) + "_value", *rateKnobs[i]));
    }
}

void MidiGeneratorEditor::setupDensityControls()
{
    // Section header
    auto* header = createLabel("DENSITY", juce::Justification::centred);
    header->setFont(juce::Font(20.0f, juce::Font::bold));
    header->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    header->setBounds(580, 20, 150, 30);
    rhythmTab->addAndMakeVisible(header);

    // Create density knob
    densityKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Overall density/probability"));
    densityKnob->setName("density");
    densityKnob->setRange(0.0, 100.0, 0.1);
    densityKnob->setTextValueSuffix("%");
    rhythmTab->addAndMakeVisible(densityKnob.get());
    densityKnob->setBounds(615, 60, 80, 100);

    // Create density label
    densityLabel = std::unique_ptr<juce::Label>(createLabel("DENSITY", juce::Justification::centred));
    densityLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    rhythmTab->addAndMakeVisible(densityLabel.get());
    densityLabel->setBounds(615, 150, 80, 30);

    // Create parameter attachment
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "density", *densityKnob));
}

void MidiGeneratorEditor::setupGateControls()
{
    // Section header
    auto* header = createLabel("GATE", juce::Justification::centred);
    header->setFont(juce::Font(20.0f, juce::Font::bold));
    header->setColour(juce::Label::textColourId, juce::Colour(0xffd952bf)); // Magenta
    header->setBounds(280, 190, 150, 30);
    rhythmTab->addAndMakeVisible(header);

    // Create gate knob
    gateKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Gate length"));
    gateKnob->setName("gate");
    gateKnob->setRange(0.0, 100.0, 0.1);
    gateKnob->setTextValueSuffix("%");
    rhythmTab->addAndMakeVisible(gateKnob.get());
    gateKnob->setBounds(230, 230, 80, 100);

    // Create gate label
    gateLabel = std::unique_ptr<juce::Label>(createLabel("GATE", juce::Justification::centred));
    gateLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    rhythmTab->addAndMakeVisible(gateLabel.get());
    gateLabel->setBounds(230, 320, 80, 30);

    // Create gate random knob
    gateRandomKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Gate randomization"));
    gateRandomKnob->setName("gate_random");
    gateRandomKnob->setRange(0.0, 100.0, 0.1);
    gateRandomKnob->setTextValueSuffix("%");
    rhythmTab->addAndMakeVisible(gateRandomKnob.get());
    gateRandomKnob->setBounds(400, 230, 80, 100);

    // Create gate random label
    gateRandomLabel = std::unique_ptr<juce::Label>(createLabel("RNDM", juce::Justification::centred));
    gateRandomLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    rhythmTab->addAndMakeVisible(gateRandomLabel.get());
    gateRandomLabel->setBounds(400, 320, 80, 30);

    // Create parameter attachments
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "gate", *gateKnob));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "gate_randomize", *gateRandomKnob));
}

void MidiGeneratorEditor::setupVelocityControls()
{
    // Section header
    auto* header = createLabel("VELO", juce::Justification::centred);
    header->setFont(juce::Font(20.0f, juce::Font::bold));
    header->setColour(juce::Label::textColourId, juce::Colour(0xffd9a652)); // Amber
    header->setBounds(580, 190, 150, 30);
    rhythmTab->addAndMakeVisible(header);

    // Create velocity knob
    velocityKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Velocity"));
    velocityKnob->setName("velocity");
    velocityKnob->setRange(0.0, 100.0, 0.1);
    velocityKnob->setTextValueSuffix("%");
    rhythmTab->addAndMakeVisible(velocityKnob.get());
    velocityKnob->setBounds(530, 230, 80, 100);

    // Create velocity label
    velocityLabel = std::unique_ptr<juce::Label>(createLabel("VELO", juce::Justification::centred));
    velocityLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    rhythmTab->addAndMakeVisible(velocityLabel.get());
    velocityLabel->setBounds(530, 320, 80, 30);

    // Create velocity random knob
    velocityRandomKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Velocity randomization"));
    velocityRandomKnob->setName("velocity_random");
    velocityRandomKnob->setRange(0.0, 100.0, 0.1);
    velocityRandomKnob->setTextValueSuffix("%");
    rhythmTab->addAndMakeVisible(velocityRandomKnob.get());
    velocityRandomKnob->setBounds(700, 230, 80, 100);

    // Create velocity random label
    velocityRandomLabel = std::unique_ptr<juce::Label>(createLabel("RNDM", juce::Justification::centred));
    velocityRandomLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    rhythmTab->addAndMakeVisible(velocityRandomLabel.get());
    velocityRandomLabel->setBounds(700, 320, 80, 30);

    // Create parameter attachments
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "velocity", *velocityKnob));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "velocity_randomize", *velocityRandomKnob));
}

void MidiGeneratorEditor::setupScaleTypeControls()
{
    // Section header
    auto* header = createLabel("SCALE", juce::Justification::centred);
    header->setFont(juce::Font(20.0f, juce::Font::bold));
    header->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    header->setBounds(230, 20, 150, 30);
    melodyTab->addAndMakeVisible(header);

    // Create scale type combo box
    scaleTypeComboBox = std::make_unique<juce::ComboBox>();
    scaleTypeComboBox->addItem("MAJOR", MidiGeneratorProcessor::SCALE_MAJOR + 1);
    scaleTypeComboBox->addItem("MINOR", MidiGeneratorProcessor::SCALE_MINOR + 1);
    scaleTypeComboBox->addItem("PENTATONIC", MidiGeneratorProcessor::SCALE_PENTATONIC + 1);
    scaleTypeComboBox->setJustificationType(juce::Justification::centred);
    scaleTypeComboBox->setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff3a3a3a));
    scaleTypeComboBox->setColour(juce::ComboBox::textColourId, juce::Colours::white);
    melodyTab->addAndMakeVisible(scaleTypeComboBox.get());
    scaleTypeComboBox->setBounds(200, 60, 200, 30);

    // Create scale label
    scaleLabel = std::unique_ptr<juce::Label>(createLabel("TYPE", juce::Justification::centred));
    scaleLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    melodyTab->addAndMakeVisible(scaleLabel.get());
    scaleLabel->setBounds(260, 100, 80, 30);

    // Create parameter attachment
    comboBoxAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.parameters, "scale_type", *scaleTypeComboBox));
}

void MidiGeneratorEditor::setupShifterControls()
{
    // No control needed yet - placeholder for future expansion
}

void MidiGeneratorEditor::setupSemitoneControls()
{
    // Section header
    auto* header = createLabel("CHANCE", juce::Justification::centred);
    header->setFont(juce::Font(20.0f, juce::Font::bold));
    header->setColour(juce::Label::textColourId, juce::Colour(0xff52d97d)); // Green
    header->setBounds(580, 20, 150, 30);
    melodyTab->addAndMakeVisible(header);

    // Create semitones knob
    semitonesKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Semitone range"));
    semitonesKnob->setName("semitones");
    semitonesKnob->setRange(0, 12, 1);
    melodyTab->addAndMakeVisible(semitonesKnob.get());
    semitonesKnob->setBounds(530, 100, 80, 100);

    // Create semitones label
    semitonesLabel = std::unique_ptr<juce::Label>(createLabel("STEPS", juce::Justification::centred));
    semitonesLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    melodyTab->addAndMakeVisible(semitonesLabel.get());
    semitonesLabel->setBounds(530, 190, 80, 30);

    // Create semitones probability knob
    semitonesProbabilityKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Semitone variation probability"));
    semitonesProbabilityKnob->setName("semitones_prob");
    semitonesProbabilityKnob->setRange(0.0, 100.0, 0.1);
    semitonesProbabilityKnob->setTextValueSuffix("%");
    melodyTab->addAndMakeVisible(semitonesProbabilityKnob.get());
    semitonesProbabilityKnob->setBounds(700, 100, 80, 100);

    // Create semitones probability label
    semitonesProbabilityLabel = std::unique_ptr<juce::Label>(createLabel("CHANCE", juce::Justification::centred));
    semitonesProbabilityLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    melodyTab->addAndMakeVisible(semitonesProbabilityLabel.get());
    semitonesProbabilityLabel->setBounds(700, 190, 80, 30);

    // Create parameter attachments
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "semitones", *semitonesKnob));
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "semitones_prob", *semitonesProbabilityKnob));
}

void MidiGeneratorEditor::setupOctaveControls()
{
    // Section header
    auto* header = createLabel("OCTAVE", juce::Justification::centred);
    header->setFont(juce::Font(20.0f, juce::Font::bold));
    header->setColour(juce::Label::textColourId, juce::Colour(0xff52d97d)); // Green
    header->setBounds(230, 190, 150, 30);
    melodyTab->addAndMakeVisible(header);

    // Create octaves knob
    octavesKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Octave range"));
    octavesKnob->setName("octaves");
    octavesKnob->setRange(0, 3, 1);
    melodyTab->addAndMakeVisible(octavesKnob.get());
    octavesKnob->setBounds(230, 230, 80, 100);

    // Create octaves label
    octavesLabel = std::unique_ptr<juce::Label>(createLabel("SHIFT", juce::Justification::centred));
    octavesLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    melodyTab->addAndMakeVisible(octavesLabel.get());
    octavesLabel->setBounds(230, 320, 80, 30);

    // Create octaves probability knob
    octavesProbabilityKnob = std::unique_ptr<juce::Slider>(createRotarySlider("Octave variation probability"));
    octavesProbabilityKnob->setName("octaves_prob");
    octavesProbabilityKnob->setRange(0.0, 100.0, 0.1);
    octavesProbabilityKnob->setTextValueSuffix("%");
    melodyTab->addAndMakeVisible(octavesProbabilityKnob.get());
    octavesProbabilityKnob->setBounds(400, 230, 80, 100);

    // Create octaves probability label
    octavesProbabilityLabel = std::unique_ptr<juce::Label>(createLabel("CHANCE", juce::Justification::centred));
    octavesProbabilityLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    melodyTab->addAndMakeVisible(octavesProbabilityLabel.get());
    octavesProbabilityLabel->setBounds(400, 320, 80, 30);

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
