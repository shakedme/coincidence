#include "MidiGeneratorEditor.h"

//==============================================================================
MidiGeneratorEditor::MidiGeneratorEditor(MidiGeneratorProcessor& p)
    : AudioProcessorEditor(&p)
    , processor(p)
{
    // Set custom look and feel
    setLookAndFeel(&customLookAndFeel);

    // Set up main sections
    grooveSection = std::make_unique<GrooveSectionComponent>(*this, processor);
    addAndMakeVisible(grooveSection.get());
    
    pitchSection = std::make_unique<PitchSectionComponent>(*this, processor);
    addAndMakeVisible(pitchSection.get());
    
    glitchSection = std::make_unique<GlitchSectionComponent>(*this, processor);
    addAndMakeVisible(glitchSection.get());
    
    sampleSection = std::make_unique<SampleSectionComponent>(*this, processor);
    addAndMakeVisible(sampleSection.get());

    // Set up keyboard
    setupKeyboard();

    // Set editor size
    setSize(800, 800);

    // Start the timer for UI updates
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
               
    // Define a helper for drawing metallic panels
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
    int sectionHeight = static_cast<int>((getHeight() - 80 - 80) * 0.45); // Reduced from 0.65 to 0.45
    int glitchHeight = (getHeight() - 80 - 80) * 0.25; // 25% of space for glitch
    int sampleHeight = (getHeight() - 80 - 80) * 0.30; // 30% of space for sample section

    int topSectionsBottom = 70 + sectionHeight;
    int glitchSectionBottom = topSectionsBottom + glitchHeight + 10;

    // Create section areas for drawing
    auto grooveArea = juce::Rectangle<int>(10, 70, getWidth() / 2 - 20, sectionHeight);
    auto pitchArea = juce::Rectangle<int>(getWidth() / 2 + 10, 70, getWidth() / 2 - 20, sectionHeight);
    auto glitchArea = juce::Rectangle<int>(10, 70 + sectionHeight + 10, getWidth() - 20, glitchHeight - 10);
    auto sampleArea = juce::Rectangle<int>(10, glitchSectionBottom, getWidth() - 20, sampleHeight - 10);

    // Draw the metallic panels for each section
    drawMetallicPanel(grooveArea, juce::Colour(0xff2a2a2a), grooveSection->getSectionColour());
    drawMetallicPanel(pitchArea, juce::Colour(0xff2a2a2a), pitchSection->getSectionColour());
    drawMetallicPanel(glitchArea, juce::Colour(0xff2a2a2a), glitchSection->getSectionColour());
    drawMetallicPanel(sampleArea, juce::Colour(0xff2a2a2a), sampleSection->getSectionColour());
}

void MidiGeneratorEditor::resized()
{
    // Calculate main sections
    auto area = getLocalBounds();
    auto headerArea = area.removeFromTop(60);
    auto mainArea = area.removeFromTop(area.getHeight() - 80);

    // Calculate dimensions for the sections with better proportions
    int topSectionHeight = static_cast<int>(mainArea.getHeight() * 0.40); // Reduced from 0.45
    int glitchHeight = mainArea.getHeight() * 0.20; // Reduced from 0.25
    int sampleHeight = mainArea.getHeight() * 0.40; // Increased from 0.30

    // Create section areas and position components with less margin between sections
    grooveSection->setBounds(10, 70, getWidth() / 2 - 20, topSectionHeight);
    pitchSection->setBounds(getWidth() / 2 + 10, 70, getWidth() / 2 - 20, topSectionHeight);

    // Move glitch section up a bit with less margin
    int glitchY = 70 + topSectionHeight + 5; // Reduced margin from 10 to 5
    glitchSection->setBounds(10, glitchY, getWidth() - 20, glitchHeight);

    // Position sample section with less margin
    int sampleY = glitchY + glitchHeight + 5; // Reduced margin from 10 to 5
    sampleSection->setBounds(10, sampleY, getWidth() - 20, sampleHeight);

    // Position the keyboard at the bottom
    const int keyboardWidth = area.getWidth() - 20;
    const int keyboardHeight = 80;
    const int keyboardY = sampleY + sampleHeight + 5; // Reduced margin
    const int keyboardX = (getWidth() - keyboardWidth) / 2;

    keyboardComponent->setBounds(keyboardX, keyboardY, keyboardWidth, keyboardHeight);
}

//==============================================================================
void MidiGeneratorEditor::setupKeyboard()
{
    // Create keyboard state
    keyboardState = std::make_unique<juce::MidiKeyboardState>();

    // Create keyboard component
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

void MidiGeneratorEditor::timerCallback()
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
    if (isShowing() && keyboardComponent != nullptr && !processor.isNoteActive()
        && keyboardState != nullptr)
    {
        keyboardState->allNotesOff(1);
        keyboardComponent->repaint();
    }
}

// FileDragAndDropTarget methods - forward to sample section
bool MidiGeneratorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    if (sampleSection)
        return sampleSection->isInterestedInFileDrag(files);
    return false;
}

void MidiGeneratorEditor::filesDropped(const juce::StringArray& files, int x, int y)
{
    if (sampleSection)
    {
        // Convert coordinates to sample section's local space
        juce::Point<int> localPoint(x, y);
        localPoint = sampleSection->getLocalPoint(this, localPoint);
        sampleSection->filesDropped(files, localPoint.x, localPoint.y);
    }
}

void MidiGeneratorEditor::fileDragEnter(const juce::StringArray& files, int x, int y)
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

void MidiGeneratorEditor::fileDragExit(const juce::StringArray& files)
{
    if (sampleSection)
        sampleSection->fileDragExit(files);
    
    isCurrentlyOver = false;
    repaint();
}
