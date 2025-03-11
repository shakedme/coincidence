#include "SampleRow.h"

SampleRow::SampleRow(SampleList *owner, int row, SamplerSound *sound)
        : ownerList(owner), rowNumber(row) {

    // Create probability slider
    slider = std::make_unique<juce::Slider>();
    slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    slider->setRange(0.0, 1.0, 0.01);
    slider->setColour(juce::Slider::thumbColourId, juce::Colour(0xffbf52d9));
    slider->setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffbf52d9));
    slider->setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff444444));
    slider->setColour(juce::Slider::trackColourId, juce::Colour(0xff222222)); // Darker track color
    slider->setColour(juce::Slider::backgroundColourId, juce::Colour(0xff666666)); // Lighter background

    // Set property to identify the slider for event handling
    slider->getProperties().set("slider", true);

    // Create icons with direct BinaryData pointers
    editIcon = std::make_unique<Icon>(BinaryData::pencil_svg, BinaryData::pencil_svgSize, 16.0f);
    deleteIcon = std::make_unique<Icon>(BinaryData::delete_svg, BinaryData::delete_svgSize, 16.0f);

    // Set icon colors
    editIcon->setNormalColour(juce::Colours::lightgrey);
    deleteIcon->setNormalColour(juce::Colours::lightgrey);

    deleteIcon->setTooltip("Delete sample");
    editIcon->setTooltip("Edit sample");
    slider->setTooltip("Sample probability");

    onsetIcon = std::make_unique<Icon>(BinaryData::threelines_svg, BinaryData::threelines_svgSize, 16.0f);
    onsetIcon->setNormalColour(juce::Colours::lightgrey);
    onsetIcon->setTooltip(
            "Toggle onset randomization - each trigger will randomize the start position based on onset in the edit view.");
    addAndMakeVisible(onsetIcon.get());

    // Add all components
    addAndMakeVisible(slider.get());
    addAndMakeVisible(editIcon.get());
    addAndMakeVisible(deleteIcon.get());

    // Setup rate icons
    setupRateIcon(rate1_1Icon, "1/1", Params::RATE_1_1);
    setupRateIcon(rate1_2Icon, "1/2", Params::RATE_1_2);
    setupRateIcon(rate1_4Icon, "1/4", Params::RATE_1_4);
    setupRateIcon(rate1_8Icon, "1/8", Params::RATE_1_8);
    setupRateIcon(rate1_16Icon, "1/16", Params::RATE_1_16);
    setupRateIcon(rate1_32Icon, "1/32", Params::RATE_1_32);

    updateContent(owner, row, sound);
}


void SampleRow::updateContent(SampleList *owner, int row, SamplerSound *sound) {
    rowNumber = row;
    ownerList = owner;
    sampleName = owner->processor.getSampleManager().getSampleName(row);

    // Add group information if applicable
    if (sound != nullptr) {
        int groupIdx = sound->getGroupIndex();
        if (groupIdx >= 0 && groupIdx < owner->processor.getSampleManager().getNumGroups()) {
            sampleName += " [G" + juce::String(groupIdx + 1) + "]";
        }
    }

    // Configure onset icon state
    if (sound != nullptr) {
        bool hasOnsetMarkers = !sound->getOnsetMarkers().empty();
        onsetIcon->setEnabled(hasOnsetMarkers);
        onsetIcon->setActive(sound->isOnsetRandomizationEnabled());
    }

    // Set initial value
    float probability = ownerList->processor.getSampleManager().getSampleProbability(row);
    slider->setValue(probability, juce::dontSendNotification);

    // Connect slider value change to the ownerList
    slider->onValueChange = [this, row]() {
        double value = slider->getValue();
        ownerList->handleSliderValueChanged(row, value);
    };

    editIcon->onClicked = [this, row]() {
        if (ownerList->onSampleDetailRequested)
            ownerList->onSampleDetailRequested(row);
    };

    onsetIcon->onClicked = [this, row]() {
        ownerList->toggleOnsetRandomization(row);
    };

    deleteIcon->onClicked = [this, row]() {
        ownerList->processor.getSampleManager().removeSamples(row, row);
        ownerList->updateContent();
    };

    // Update rate icons
    updateRateIcon(Params::RATE_1_1);
    updateRateIcon(Params::RATE_1_2);
    updateRateIcon(Params::RATE_1_4);
    updateRateIcon(Params::RATE_1_8);
    updateRateIcon(Params::RATE_1_16);
    updateRateIcon(Params::RATE_1_32);
}

void SampleRow::paint(juce::Graphics &g) {
    static const juce::Font sampleNameFont(juce::FontOptions(14.0f));
    g.setColour(juce::Colours::white);
    g.setFont(sampleNameFont);

    // Use minimal padding between controls
    static const int iconSize = 16;
    static const int padding = 2; // Reduced from 8 to 2
    static const int sliderSize = 16;
    static const int numRateIcons = 6;
    static const int numOtherIcons = 4;

    // Calculate minimum width needed for controls with minimal padding
    static const int controlsWidth = numRateIcons * rateIconWidth +
                                     numOtherIcons * iconSize +
                                     sliderSize +
                                     (numRateIcons + numOtherIcons + 1) * padding +
                                     4; // Extra right margin

    // Calculate text area width based on available space
    const int textAreaWidth = juce::jmax(50, getWidth() - controlsWidth - 8);

    // Draw the sample name with calculated width
    g.drawText(sampleName,
               4, // Left margin
               0,
               textAreaWidth,
               getHeight(),
               juce::Justification::centredLeft);
}

void SampleRow::resized() {
    static const int iconSize = 16;
    static const int sliderSize = 16;
    static const int padding = 4; // Reduced padding

    // Calculate total available width
    const int availableWidth = getWidth();

    // Start position for right-aligned elements (with small right margin)
    int x = availableWidth - iconSize - 4; // 4px right margin
    const int yCenter = (getHeight() - iconSize) / 2;

    // Position all controls from right to left with minimal padding
    deleteIcon->setBounds(x, yCenter, iconSize, iconSize);

    x -= iconSize + padding;
    editIcon->setBounds(x, yCenter, iconSize, iconSize);

    x -= iconSize + padding;
    onsetIcon->setBounds(x, yCenter, iconSize, iconSize);

    x -= sliderSize + padding;
    slider->setBounds(x, yCenter, sliderSize, sliderSize);

    // Position rate icons with minimal padding
    x -= rateIconWidth + padding;
    rate1_32Icon->setBounds(x, yCenter, rateIconWidth, iconSize);

    x -= rateIconWidth + padding;
    rate1_16Icon->setBounds(x, yCenter, rateIconWidth, iconSize);

    x -= rateIconWidth + padding;
    rate1_8Icon->setBounds(x, yCenter, rateIconWidth, iconSize);

    x -= rateIconWidth + padding;
    rate1_4Icon->setBounds(x, yCenter, rateIconWidth, iconSize);

    x -= rateIconWidth + padding;
    rate1_2Icon->setBounds(x, yCenter, rateIconWidth, iconSize);

    x -= rateIconWidth + padding;
    rate1_1Icon->setBounds(x, yCenter, rateIconWidth, iconSize);
}

void SampleRow::mouseDown(const juce::MouseEvent &e) {
    // Only forward mouse events if not clicking on an icon
    if (!e.eventComponent->getProperties().contains("icon")) {
        // Forward the event to the parent component for selection handling
        if (juce::Component *parent = getParentComponent())
            parent->mouseDown(e.getEventRelativeTo(parent));
    }
}

void SampleRow::mouseUp(const juce::MouseEvent &e) {
    // Only forward mouse events if not clicking on an icon
    if (!e.eventComponent->getProperties().contains("icon")) {
        // Forward the event to the parent component
        if (juce::Component *parent = getParentComponent())
            parent->mouseUp(e.getEventRelativeTo(parent));
    }
}

void SampleRow::mouseDrag(const juce::MouseEvent &e) {
    // Only forward mouse events if not clicking on an icon
    if (!e.eventComponent->getProperties().contains("icon")) {
        // For drag selection, we need to forward this to the table list box
        if (juce::Component *parent = getParentComponent()) {
            auto parentEvent = e.getEventRelativeTo(parent);

            // Find table list box parent component
            auto *tableComp = dynamic_cast<juce::TableListBox *>(parent->getParentComponent());
            if (tableComp != nullptr) {
                // If using Shift or Ctrl/Cmd, extend selection to the row under mouse
                if (e.mods.isShiftDown() || e.mods.isCommandDown() || e.mods.isCtrlDown()) {
                    // Get row index under current mouse position
                    auto relPos = parentEvent.getPosition();
                    int rowUnderMouse = tableComp->getRowContainingPosition(relPos.x, relPos.y);

                    if (rowUnderMouse >= 0) {
                        // If shift is down, select range
                        if (e.mods.isShiftDown()) {
                            // Get the anchor (first selected row)
                            auto selectedRows = tableComp->getSelectedRows();
                            int anchorRow = selectedRows.isEmpty() ? rowNumber : selectedRows[0];

                            // Select range from anchor to current row
                            tableComp->selectRangeOfRows(juce::jmin(anchorRow, rowUnderMouse),
                                                         juce::jmax(anchorRow, rowUnderMouse));
                        }
                            // If Ctrl/Cmd is down, add to selection
                        else if (e.mods.isCommandDown() || e.mods.isCtrlDown()) {
                            tableComp->selectRow(rowUnderMouse, true); // Add to selection
                        }
                    }
                }
            }

            // Forward the drag event
            parent->mouseDrag(parentEvent);
        }
    }
}

void SampleRow::setupRateIcon(std::unique_ptr<TextIcon> &icon,
                              const juce::String &text, Params::RateOption /*rate*/) {
    icon = std::make_unique<TextIcon>(text, rateIconWidth, 16.0f);
    icon->setNormalColour(juce::Colours::lightgrey);
    icon->setTooltip("Toggle " + text + " rate");
    addAndMakeVisible(icon.get());
}

void SampleRow::updateRateIcon(Params::RateOption rate) {
    if (auto *icon = getRateIcon(rate)) {
        bool isEnabled = ownerList->processor.getSampleManager().isSampleRateEnabled(rowNumber, rate);
        if (isEnabled) {
            icon->setActive(true, juce::Colour(0xff52bfd9));
        } else {
            icon->setActive(false);
        }
        // Add click handler
        icon->onClicked = [this, icon, rate]() {
            bool currentState = ownerList->processor.getSampleManager().isSampleRateEnabled(rowNumber, rate);
            ownerList->processor.getSampleManager().setSampleRateEnabled(rowNumber, rate, !currentState);
            if (!currentState) {
                icon->setActive(true, juce::Colour(0xff52bfd9));
            } else {
                icon->setActive(false);
            }
        };
    }
}

TextIcon *SampleRow::getRateIcon(Params::RateOption rate) {
    switch (rate) {
        case Params::RATE_1_1:
            return rate1_1Icon.get();
        case Params::RATE_1_2:
            return rate1_2Icon.get();
        case Params::RATE_1_4:
            return rate1_4Icon.get();
        case Params::RATE_1_8:
            return rate1_8Icon.get();
        case Params::RATE_1_16:
            return rate1_16Icon.get();
        case Params::RATE_1_32:
            return rate1_32Icon.get();
        default:
            return nullptr;
    }
}