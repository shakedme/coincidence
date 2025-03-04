#include "PluginEditor.h"

//==============================================================================
PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p)
    , audioProcessor(p)
{
    // Set custom look and feel
    setLookAndFeel(&customLookAndFeel);

    // Set up main sections
    grooveSection = std::make_unique<GrooveSectionComponent>(*this, audioProcessor);
    addAndMakeVisible(grooveSection.get());

    pitchSection = std::make_unique<PitchSectionComponent>(*this, audioProcessor);
    addAndMakeVisible(pitchSection.get());

    glitchSection = std::make_unique<GlitchSectionComponent>(*this, audioProcessor);
    addAndMakeVisible(glitchSection.get());

    sampleSection = std::make_unique<SampleSectionComponent>(*this, audioProcessor);
    addAndMakeVisible(sampleSection.get());

    // Set up keyboard
    setupKeyboard();

    // Set editor size
    setSize(800, 700);

    // Start the timer for UI updates
    startTimerHz(30);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

//==============================================================================
void PluginEditor::paint(juce::Graphics& g)
{
    // Fill background with dark grey
    g.fillAll(juce::Colour(0xff222222));

    //    // Draw title banner with a metallic gradient
    //    g.setGradientFill(juce::ColourGradient(
    //        juce::Colour(0xff505050), 0, 0, juce::Colour(0xff303030), 0, 60, false));
    //    g.fillRect(0, 0, getWidth(), 30);
    //
    //    // Add subtle highlight to give a metal panel effect
    //    g.setColour(juce::Colour(0x20ffffff));
    //    g.fillRect(0, 3, getWidth(), 2);
    //
    //    // Draw plugin logo/title
    //    g.setColour(juce::Colours::white);
    //    g.setFont(juce::Font(20.0f, juce::Font::bold));
    //    g.drawText(
    //        "Jammer", getLocalBounds().removeFromTop(30), juce::Justification::centred, true);
}

void PluginEditor::resized()
{
    // Calculate main sections
    auto area = getLocalBounds();

    // Calculate dimensions for the sections with better proportions
    int topSectionY = 10;
    int sectionPadding = 5;
    int xPadding = 10;
    int topSectionHeight = static_cast<int>(area.getHeight() * 0.40);
    int glitchHeight = static_cast<int>(area.getHeight() * 0.20);
    int sampleHeight = static_cast<int>(area.getHeight() * 0.30);
    int grooveWidth = static_cast<int>(getWidth() * 0.7f) - 15;
    int pitchWidth = getWidth() - grooveWidth - 25;
    int pitchX = xPadding + grooveWidth + sectionPadding;

    grooveSection->setBounds(xPadding, topSectionY, grooveWidth, topSectionHeight);
    pitchSection->setBounds(pitchX, topSectionY, pitchWidth, topSectionHeight);

    int sampleY = topSectionY + topSectionHeight + sectionPadding;
    sampleSection->setBounds(xPadding, sampleY, getWidth() - xPadding * 2, sampleHeight);

    int glitchY = sampleY + sampleHeight + sectionPadding;
    glitchSection->setBounds(xPadding, glitchY, getWidth() - xPadding * 2, glitchHeight);

    // Position the keyboard at the bottom
    const int keyboardWidth = area.getWidth() - 20;
    const int keyboardHeight = 40;
    const int keyboardY = glitchY + glitchHeight + 5;
    const int keyboardX = (getWidth() - keyboardWidth) / 2;

    keyboardComponent->setBounds(keyboardX, keyboardY, keyboardWidth, keyboardHeight);
}
//==============================================================================
void PluginEditor::setupKeyboard()
{
    keyboardState = std::make_unique<juce::MidiKeyboardState>();
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

    addAndMakeVisible(keyboardComponent.get());
}

void PluginEditor::updateKeyboardState(bool isNoteOn, int noteNumber, int velocity)
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

void PluginEditor::timerCallback()
{
    // Update groove section for knob randomization visualization
    if (grooveSection)
    {
        grooveSection->repaintRandomizationControls();
        grooveSection->updateRateLabelsForRhythmMode();
    }

    // Update keyboard display if needed
    if (keyboardNeedsUpdate && keyboardComponent != nullptr)
    {
        keyboardComponent->repaint();
        keyboardNeedsUpdate = false;
    }

    // Handle any stray notes by making sure keyboard state is correct
    if (isShowing() && keyboardComponent != nullptr && !audioProcessor.isNoteActive()
        && keyboardState != nullptr)
    {
        keyboardState->allNotesOff(1);
        keyboardComponent->repaint();
    }
}

bool PluginEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    if (sampleSection)
        return sampleSection->isInterestedInFileDrag(files);
    return false;
}

void PluginEditor::filesDropped(const juce::StringArray& files, int x, int y)
{
    if (sampleSection)
    {
        // Convert coordinates to sample section's local space
        juce::Point<int> localPoint(x, y);
        localPoint = sampleSection->getLocalPoint(this, localPoint);
        sampleSection->filesDropped(files, localPoint.x, localPoint.y);
    }
}

void PluginEditor::fileDragEnter(const juce::StringArray& files, int x, int y)
{
    if (sampleSection)
    {
        // Convert coordinates to sample section's local space
        juce::Point<int> localPoint(x, y);
        localPoint = sampleSection->getLocalPoint(this, localPoint);
        sampleSection->fileDragEnter(files, localPoint.x, localPoint.y);
    }

    isCurrentlyOver = true;
    repaint();
}

void PluginEditor::fileDragExit(const juce::StringArray& files)
{
    if (sampleSection)
        sampleSection->fileDragExit(files);

    isCurrentlyOver = false;
    repaint();
}
