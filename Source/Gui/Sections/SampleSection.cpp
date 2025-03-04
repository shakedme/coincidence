#include "SampleSection.h"

SampleSectionComponent::SampleSectionComponent(PluginEditor& editor,
                                               PluginProcessor& processor)
    : BaseSectionComponent(editor, processor, "SAMPLE", juce::Colour(0xffbf52d9))
{
    initComponents(processor);
    startTimerHz(30);
}

SampleSectionComponent::~SampleSectionComponent()
{
    stopTimer();
    clearAttachments();
}

void SampleSectionComponent::resized()
{
    auto area = getLocalBounds();

    // Sample section layout
    int controlsY = 40;

    // Sample list area - takes up left side
    int sampleListWidth = area.getWidth() * 0.6f;
    int sampleListHeight = area.getHeight() - 70;
    juce::Rectangle<int> listArea(area.getX() + 10, controlsY, sampleListWidth, sampleListHeight);

    // Set both the list box and detail view to occupy the same space for seamless transition
    sampleList->setBounds(listArea);
    sampleDetailView->setBounds(listArea);

    // Sample name display
    int sampleNameY = controlsY + area.getHeight() - 95;
    sampleNameLabel->setBounds(area.getX(), sampleNameY, sampleListWidth, 25);

    // Right side controls - direction selector
    int controlsX = area.getX() + sampleListWidth + 15;
    int controlsWidth = area.getWidth() - sampleListWidth - 25;

    // Position the sample direction selector in the center of right panel
    sampleDirectionSelector->setBounds(
        controlsX + (controlsWidth - 80) / 2, // Center it horizontally
        controlsY + 60, // Place it in the middle vertically
        80, // Width
        25); // Height
}

void SampleSectionComponent::initComponents(PluginProcessor& processor)
{
    // Create the sample list component
    sampleList = std::make_unique<SampleList>(processor);
    sampleList->onSampleDetailRequested = [this](int sampleIndex) {
        showDetailViewForSample(sampleIndex);
    };
    addAndMakeVisible(sampleList.get());

    // Create the detail view component but don't make it visible yet
    sampleDetailView = std::make_unique<SampleDetailComponent>(processor.getSampleManager());
    sampleDetailView->onBackButtonClicked = [this]() { showListView(); };
    addChildComponent(sampleDetailView.get()); // Add as child but not visible

    removeSampleButton = std::make_unique<juce::TextButton>("Remove");
    addAndMakeVisible(removeSampleButton.get());

    // Sample direction selector
    sampleDirectionSelector = std::make_unique<DirectionSelector>(juce::Colour(0xffbf52d9));

    // Set initial value from parameter
    auto* sampleDirectionParam = dynamic_cast<juce::AudioParameterChoice*>(
        processor.parameters.getParameter("sample_direction"));

    if (sampleDirectionParam)
        sampleDirectionSelector->setDirection(
            static_cast<Params::DirectionType>(sampleDirectionParam->getIndex()));

    sampleDirectionSelector->onDirectionChanged =
        [this, &processor](Params::DirectionType direction)
    {
        auto* param = processor.parameters.getParameter("sample_direction");
        if (param)
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<int>(direction)));
            param->endChangeGesture();
        }
    };
    addAndMakeVisible(sampleDirectionSelector.get());

    sampleNameLabel = std::unique_ptr<juce::Label>(createLabel("", juce::Justification::centred));
    sampleNameLabel->setFont(juce::Font(11.0f));
    addAndMakeVisible(sampleNameLabel.get());
}

void SampleSectionComponent::paint(juce::Graphics& g)
{
    // Call the base class paint method first to ensure we get the section header
    BaseSectionComponent::paint(g);

    // Check if we have any samples loaded
    bool noSamplesLoaded = processor.getSampleManager().getNumSamples() == 0;

    // Get the content area (excluding the header area)
    auto contentArea = getLocalBounds().withTrimmedTop(40);

    // If no samples are loaded and not showing detail view, handle drop zone
    if (noSamplesLoaded && !showingDetailView)
    {
        if (draggedOver)
        {
            g.setColour(juce::Colours::white.withAlpha(0.2f));
            g.drawRect(contentArea.reduced(10), 2);
        }

        // Hide the sample direction selector when no samples are loaded
        sampleDirectionSelector->setVisible(false);
    }
    else
    {
        // Show the sample direction selector when samples are loaded
        sampleDirectionSelector->setVisible(true);
    }
}

void SampleSectionComponent::timerCallback()
{
    // Get the currently active sample index from the processor
    int currentActiveSample = processor.getNoteGenerator().getCurrentActiveSampleIdx();

    // If the active sample has changed, update the display
    if (currentActiveSample != lastActiveSampleIndex)
    {
        lastActiveSampleIndex = currentActiveSample;

        if (!showingDetailView)
        {
            // Update the sample list's active index
            sampleList->setActiveSampleIndex(currentActiveSample);

            // Update the sample name label if a valid sample is playing
            if (currentActiveSample >= 0 && currentActiveSample < processor.getSampleManager().getNumSamples())
            {
                sampleNameLabel->setText("Playing: " + processor.getSampleManager().getSampleName(currentActiveSample),
                                         juce::dontSendNotification);
            }
            else
            {
                sampleNameLabel->setText("", juce::dontSendNotification);
            }
        }
    }
}

bool SampleSectionComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    // Only accept file drags when showing the list view
    if (showingDetailView)
        return false;

    // Check if any of the files are audio files
    for (int i = 0; i < files.size(); ++i)
    {
        juce::File f(files[i]);
        if (f.hasFileExtension("wav;aif;aiff;mp3;flac;ogg;m4a;wma"))
            return true;
    }
    return false;
}

void SampleSectionComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    // Reset the drag state
    draggedOver = false;

    // Process the dropped files
    bool needsUpdate = false;

    for (const auto& file: files)
    {
        juce::File f(file);
        if (f.existsAsFile() && f.hasFileExtension("wav;aif;aiff;mp3"))
        {
            processor.getSampleManager().addSample(f);
            needsUpdate = true;
        }
    }

    if (needsUpdate)
    {
        sampleList->updateContent();
        repaint();
    }

    // Update sample name label
    if (processor.getSampleManager().getNumSamples() == 0)
    {
        sampleNameLabel->setText("No samples loaded", juce::dontSendNotification);
    }
    else
    {
        sampleNameLabel->setText("", juce::dontSendNotification);
    }
}

void SampleSectionComponent::fileDragEnter(const juce::StringArray& files, int x, int y)
{
    // Check if any of the files are valid audio files
    bool hasValidFiles = false;
    for (const auto& file: files)
    {
        juce::File f(file);
        if (f.hasFileExtension("wav;aif;aiff;mp3;ogg;flac"))
        {
            hasValidFiles = true;
            break;
        }
    }

    if (hasValidFiles)
    {
        draggedOver = true;
        repaint();
    }
}

void SampleSectionComponent::fileDragExit(const juce::StringArray& files)
{
    draggedOver = false;
    repaint();
}

void SampleSectionComponent::showListView()
{
    // Hide detail view, show list view
    sampleDetailView->setVisible(false);
    sampleList->setVisible(true);

    // Update state
    showingDetailView = false;

    // Force a full layout refresh
    resized();
    repaint();
}

void SampleSectionComponent::showDetailViewForSample(int sampleIndex)
{
    if (sampleDetailView->getSampleIndex() != sampleIndex) {
        sampleDetailView->clearSampleData();
    }

    if (sampleIndex >= 0 && sampleIndex < processor.getSampleManager().getNumSamples())
    {
        // Set up detail view for this sample
        sampleDetailView->setSampleIndex(sampleIndex);

        // Show detail view, hide list view
        sampleList->setVisible(false);
        sampleDetailView->setVisible(true);

        // Update state
        showingDetailView = true;

        repaint();
    }
}