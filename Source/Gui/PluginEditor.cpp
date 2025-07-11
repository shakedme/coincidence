#include "PluginEditor.h"

PluginEditor::PluginEditor(PluginProcessor &p)
        : AudioProcessorEditor(&p), audioProcessor(p) {
    setLookAndFeel(&customLookAndFeel);

    header = std::make_unique<HeaderComponent>(audioProcessor);
    header->onTabChanged = [this](HeaderComponent::Tab tab) { switchTab(tab); };
    addAndMakeVisible(header.get());

    grooveSection = std::make_unique<GrooveSectionComponent>(*this, audioProcessor);
    addAndMakeVisible(grooveSection.get());

    pitchSection = std::make_unique<PitchSectionComponent>(*this, audioProcessor);
    addAndMakeVisible(pitchSection.get());

    sampleSection = std::make_unique<SampleSectionComponent>(*this, audioProcessor);
    addAndMakeVisible(sampleSection.get());

    envelopeSection = std::make_unique<EnvelopeSection>(*this, audioProcessor);
    addAndMakeVisible(envelopeSection.get());

    fxSection = std::make_unique<EffectsSection>(*this, audioProcessor);
    addChildComponent(fxSection.get());

    tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 0);

    setupKeyboard();

    setSize(800, 800);

    startTimerHz(30);
}

PluginEditor::~PluginEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}


void PluginEditor::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(0xff222222));
}

void PluginEditor::resized() {
    const int headerHeight = 40;
    header->setBounds(0, 0, getWidth(), headerHeight);

    auto area = getLocalBounds().withTrimmedTop(headerHeight);

    const int keyboardWidth = area.getWidth() - 20;
    const int keyboardHeight = 40;
    const int keyboardX = (getWidth() - keyboardWidth) / 2;
    const int keyboardY = area.getBottom() - keyboardHeight - 10;

    keyboardComponent->setBounds(keyboardX, keyboardY, keyboardWidth, keyboardHeight);

    auto contentArea = area.withTrimmedBottom(keyboardHeight + 15);

    int topSectionY = contentArea.getY();
    int sectionPadding = 5;
    int xPadding = 10;
    int envelopesHeight = static_cast<int>(contentArea.getHeight() * 0.37);
    int sampleHeight = static_cast<int>(contentArea.getHeight() * 0.25);
    int grooveWidth = static_cast<int>(getWidth() * 0.7f) - 15;
    int pitchWidth = getWidth() - grooveWidth - 25;
    int pitchX = xPadding + grooveWidth + sectionPadding;
    int topSectionHeight = static_cast<int>(contentArea.getHeight() * 0.38);
    int sampleY = topSectionY + topSectionHeight + sectionPadding;

    if (fxSection->isVisible()) {
        fxSection->setBounds(xPadding, topSectionY,
                             getWidth() - xPadding * 2, getHeight() - envelopesHeight - keyboardHeight - headerHeight);
    } else {
        grooveSection->setBounds(xPadding, topSectionY, grooveWidth, topSectionHeight);
        pitchSection->setBounds(pitchX, topSectionY, pitchWidth, topSectionHeight);
        sampleSection->setBounds(xPadding, sampleY, getWidth() - xPadding * 2, sampleHeight);
    }

    int fxY = sampleY + sampleHeight + sectionPadding;
    envelopeSection->setBounds(10, fxY, getWidth() - 20, envelopesHeight);
}

//==============================================================================
void PluginEditor::setupKeyboard() {
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

void PluginEditor::updateKeyboardState(bool isNoteOn, int noteNumber, int velocity) {
    if (keyboardState != nullptr) {
        if (isNoteOn)
            keyboardState->noteOn(1, noteNumber, static_cast<float>(velocity) / 127.0f);
        else
            keyboardState->noteOff(1, noteNumber, 0.0f);

        keyboardNeedsUpdate = true;
    }
}

void PluginEditor::setWaveformAudioBuffer(const float *audioData, int numSamples) {
    if (envelopeSection != nullptr)
        envelopeSection->pushAudioBuffer(audioData, numSamples);
}

void PluginEditor::switchTab(HeaderComponent::Tab tab) {
    bool isMainTab = (tab == HeaderComponent::Tab::Main);

    grooveSection->setVisible(isMainTab);
    pitchSection->setVisible(isMainTab);
    sampleSection->setVisible(isMainTab);

    fxSection->setVisible(!isMainTab);

    resized();
}

void PluginEditor::timerCallback() {
    if (grooveSection && grooveSection->isVisible()) {
        grooveSection->repaintRandomizationControls();
        grooveSection->updateRateLabelsForRhythmMode();
    }

    if (keyboardNeedsUpdate && keyboardComponent != nullptr) {
        keyboardComponent->repaint();
        keyboardNeedsUpdate = false;
    }

    if (isShowing() && keyboardComponent != nullptr && !audioProcessor.isNoteActive()
        && keyboardState != nullptr) {
        keyboardState->allNotesOff(1);
        keyboardComponent->repaint();
    }
}

bool PluginEditor::isInterestedInFileDrag(const juce::StringArray &files) {
    if (sampleSection && sampleSection->isVisible())
        return sampleSection->isInterestedInFileDrag(files);
    return false;
}

void PluginEditor::filesDropped(const juce::StringArray &files, int x, int y) {
    if (sampleSection && sampleSection->isVisible()) {
        // Convert coordinates to sample section's local space
        juce::Point<int> localPoint(x, y);
        localPoint = sampleSection->getLocalPoint(this, localPoint);
        sampleSection->filesDropped(files, localPoint.x, localPoint.y);
    }
}

void PluginEditor::fileDragEnter(const juce::StringArray &files, int x, int y) {
    if (sampleSection && sampleSection->isVisible()) {
        // Convert coordinates to sample section's local space
        juce::Point<int> localPoint(x, y);
        localPoint = sampleSection->getLocalPoint(this, localPoint);
        sampleSection->fileDragEnter(files, localPoint.x, localPoint.y);
    }

    repaint();
}

void PluginEditor::fileDragExit(const juce::StringArray &files) {
    if (sampleSection && sampleSection->isVisible())
        sampleSection->fileDragExit(files);
    repaint();
}
