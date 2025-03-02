//
// Created by Shaked Melman on 01/03/2025.
//

#include "SampleSection.h"

SampleSectionComponent::SampleSectionComponent(PluginEditor& editor,
                                               PluginProcessor& processor)
    : BaseSectionComponent(editor, processor, "SAMPLE", juce::Colour(0xffbf52d9))
{
    // Create sample list table
    sampleListBox = std::make_unique<juce::TableListBox>("Sample List", this);
    sampleListBox->setHeaderHeight(20); // Reduced from 22

    // Make the list box transparent
    sampleListBox->setColour(juce::ListBox::backgroundColourId,
                             juce::Colours::transparentBlack);
    sampleListBox->setColour(juce::ListBox::outlineColourId,
                             juce::Colours::transparentBlack);
    sampleListBox->setColour(juce::TableListBox::backgroundColourId,
                             juce::Colours::transparentBlack);

    // Make header background transparent
    sampleListBox->getHeader().setColour(juce::TableHeaderComponent::backgroundColourId,
                                         juce::Colours::transparentBlack);
    sampleListBox->getHeader().setColour(juce::TableHeaderComponent::outlineColourId,
                                         juce::Colour(0xff4a4a4a));
    sampleListBox->getHeader().setColour(juce::TableHeaderComponent::textColourId,
                                         juce::Colours::white);

    // Add column with a better name that won't overflow
    sampleListBox->getHeader().addColumn("Samples", 1, 200);
    addAndMakeVisible(sampleListBox.get());

    // Sample management buttons
    addSampleButton = std::make_unique<juce::TextButton>("Add");
    //    addSampleButton->onClick = [this]()
    //    {
    //        juce::FileChooser chooser(
    //            "Select a sample to load...",
    //            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
    //            "*.wav;*.aif;*.aiff");
    //
    //        chooser.launchAsync(juce::FileBrowserComponent::openMode,
    //                            [this](const juce::FileChooser& chooser)
    //                            {
    //                                if (chooser.getResult().existsAsFile())
    //                                {
    //                                    auto file = chooser.getResult();
    //                                    processor.addSample(file);
    //                                    sampleListBox->updateContent();
    //                                }
    //                            });
    //    };
    addAndMakeVisible(addSampleButton.get());

    removeSampleButton = std::make_unique<juce::TextButton>("Remove");
    //    removeSampleButton->onClick = [this]()
    //    {
    //        int selectedRow = sampleListBox->getSelectedRow();
    //        if (selectedRow >= 0)
    //        {
    //            processor.removeSample(selectedRow);
    //            sampleListBox->updateContent();
    //        }
    //    };
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
    sampleNameLabel->setFont(juce::Font(11.0f));
    addAndMakeVisible(sampleNameLabel.get());

    // Add parameter attachments for controls
    buttonAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.parameters, "randomize_samples", *randomizeToggle));

    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "randomize_probability", *randomizeProbabilitySlider));
}

SampleSectionComponent::~SampleSectionComponent()
{
    clearAttachments();
}

void SampleSectionComponent::resized()
{
    auto area = getLocalBounds();

    // Set up header
//    sectionLabel->setBounds(area.getX(), 57, area.getWidth(), 25); // Reduced from 30

    // Sample section layout
    int controlsY = 30; // Reduced from 40

    // Sample list takes up left side
    int sampleListWidth = area.getWidth() * 0.6f;
    sampleListBox->setBounds(
        area.getX(), controlsY, sampleListWidth, area.getHeight() - 70); // Reduced margin

    // Buttons below the sample list
    int buttonY = controlsY + area.getHeight() - 65; // Moved up
    int buttonWidth = 75; // Reduced from 80
    addSampleButton->setBounds(area.getX(), buttonY, buttonWidth, 25);
    removeSampleButton->setBounds(
        area.getX() + buttonWidth + 5, buttonY, buttonWidth, 25);

    // Sample name display
    sampleNameLabel->setBounds(area.getX(), buttonY - 30, sampleListWidth, 25);

    // Right side controls
    int controlsX = area.getX() + sampleListWidth + 15; // Reduced from 20
    int controlsWidth = area.getWidth() - sampleListWidth - 25; // Reduced margin

    randomizeToggle->setBounds(controlsX, controlsY, controlsWidth, 25);

    randomizeProbabilityLabel->setBounds(controlsX, controlsY + 35, 75, 25); // Moved up
    randomizeProbabilitySlider->setBounds(
        controlsX + 80, controlsY + 35, controlsWidth - 90, 25); // Reduced margin
}

void SampleSectionComponent::paint(juce::Graphics& g)
{
    // Call the base class paint method first to ensure we get the section header
    BaseSectionComponent::paint(g);

    // Check if we have any samples loaded
    bool noSamplesLoaded = processor.getSampleManager().getNumSamples() == 0;

    // Get the content area (excluding the header area)
    auto contentArea = getLocalBounds().withTrimmedTop(40);

    if (noSamplesLoaded || isCurrentlyOver)
    {
        // Draw the metallic drop area that resembles the styling of other sections
        auto dropArea = contentArea.reduced(15, 15);

        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.setColour(isCurrentlyOver ? getSectionColour()
                                    : juce::Colours::white.withAlpha(0.8f));

        juce::String message = "Please drop a sample";
        if (isCurrentlyOver)
            message = "Release to add sample";

        g.drawText(message, dropArea, juce::Justification::centred, true);

        // Add format hint text
        g.setFont(juce::Font(11.0f));
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.drawText("(.wav, .aif, .mp3, etc.)",
                   dropArea.withTrimmedTop(40),
                   juce::Justification::centredTop,
                   true);

        // Add drop arrow icon when dragging over
        if (isCurrentlyOver)
        {
            int arrowWidth = 40;
            int arrowHeight = 25;
            int centerX = dropArea.getCentreX();
            int centerY = dropArea.getCentreY() + 35;

            juce::Path arrow;
            arrow.startNewSubPath(centerX - arrowWidth / 2, centerY - arrowHeight);
            arrow.lineTo(centerX + arrowWidth / 2, centerY - arrowHeight);
            arrow.lineTo(centerX, centerY);
            arrow.closeSubPath();

            // Use a metallic gradient for the arrow to match the plugin style
            g.setGradientFill(juce::ColourGradient(getSectionColour().brighter(0.2f),
                                                   centerX,
                                                   centerY - arrowHeight,
                                                   getSectionColour().darker(0.2f),
                                                   centerX,
                                                   centerY,
                                                   false));
            g.fillPath(arrow);

            // Add a highlight edge
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.strokePath(arrow, juce::PathStrokeType(1.0f));
        }

        // Hide the actual components when no samples are loaded
        if (noSamplesLoaded)
        {
            sampleListBox->setVisible(false);
            addSampleButton->setVisible(false);
            removeSampleButton->setVisible(false);
            randomizeToggle->setVisible(false);
            randomizeProbabilitySlider->setVisible(false);
            randomizeProbabilityLabel->setVisible(false);
            sampleNameLabel->setVisible(false);
        }
    }
    else
    {
        // When samples are loaded, ensure controls are visible
        sampleListBox->setVisible(true);
        addSampleButton->setVisible(true);
        removeSampleButton->setVisible(true);
        randomizeToggle->setVisible(true);
        randomizeProbabilitySlider->setVisible(true);
        randomizeProbabilityLabel->setVisible(true);
        sampleNameLabel->setVisible(true);

        // Draw a subtle metallic background for the list area
        auto listArea = contentArea.withTrimmedRight(contentArea.getWidth() * 0.4);

        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3a3a3a),
                                               listArea.getX(),
                                               listArea.getY(),
                                               juce::Colour(0xff2a2a2a),
                                               listArea.getX(),
                                               listArea.getBottom(),
                                               false));
        g.fillRoundedRectangle(listArea.reduced(5).toFloat(), 4.0f);
        g.setColour(juce::Colour(0xff4a4a4a));
        g.drawRoundedRectangle(listArea.reduced(5).toFloat(), 4.0f, 1.0f);

        // Draw a metallic background for the controls area
        auto controlsArea = contentArea.withTrimmedLeft(contentArea.getWidth() * 0.6 + 5);

        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3a3a3a),
                                               controlsArea.getX(),
                                               controlsArea.getY(),
                                               juce::Colour(0xff2a2a2a),
                                               controlsArea.getX(),
                                               controlsArea.getBottom(),
                                               false));
        g.fillRoundedRectangle(controlsArea.reduced(5).toFloat(), 4.0f);
        g.setColour(juce::Colour(0xff4a4a4a));
        g.drawRoundedRectangle(controlsArea.reduced(5).toFloat(), 4.0f, 1.0f);
    }
}

int SampleSectionComponent::getNumRows()
{
    return processor.getSampleManager().getNumSamples();
}

void SampleSectionComponent::paintRowBackground(
    juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colour(0x80bf52d9)); // Purple highlight
    else if (rowNumber % 2)
        g.fillAll(juce::Colour(0xff3a3a3a)); // Darker
    else
        g.fillAll(juce::Colour(0xff444444)); // Lighter
}

void SampleSectionComponent::paintCell(juce::Graphics& g,
                                       int rowNumber,
                                       int columnId,
                                       int width,
                                       int height,
                                       bool rowIsSelected)
{
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);

    if (rowNumber < processor.getSampleManager().getNumSamples())
    {
        if (columnId == 1) // Sample name
        {
            g.drawText(processor.getSampleManager().getSampleName(rowNumber),
                       2,
                       0,
                       width - 4,
                       height,
                       juce::Justification::centredLeft);
        }
    }
}

void SampleSectionComponent::cellClicked(int rowNumber,
                                         int columnId,
                                         const juce::MouseEvent&)
{
    if (rowNumber < processor.getSampleManager().getNumSamples())
    {
        if (columnId == 1) // Sample name - select this sample
        {
            processor.getSampleManager().selectSample(rowNumber);
        }
    }
}

bool SampleSectionComponent::isInterestedInFileDrag(const juce::StringArray& files)
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

void SampleSectionComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    // First, reset the drag state
    isCurrentlyOver = false;

    // Process the dropped files
    bool needsUpdate = false;

    for (const auto& file: files)
    {
        juce::File f(file);
        if (f.existsAsFile() && f.hasFileExtension("wav;aif;aiff;mp3"))
        {
            DBG("Loading dropped sample: " + f.getFullPathName());
            processor.getSampleManager().addSample(f);
            needsUpdate = true;
        }
    }

    if (needsUpdate)
    {
        sampleListBox->updateContent();
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
    if (isInterestedInFileDrag(files))
    {
        isCurrentlyOver = true;
        repaint();
    }
}

void SampleSectionComponent::fileDragExit(const juce::StringArray& files)
{
    isCurrentlyOver = false;
    repaint();
}