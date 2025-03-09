#include <juce_audio_utils/juce_audio_utils.h>
#include "SampleList.h"
#include "../../Audio/Sampler/Sampler.h"

// A complete cell component that includes both sample name, probability slider and icons
class SampleNameCellComponent : public juce::Component {
public:
    SampleNameCellComponent(SampleList *owner, int row, SamplerSound *sound)
            : ownerList(owner), rowNumber(row) {
        // Get the sample name
        sampleName = owner->processor.getSampleManager().getSampleName(row);

        // Add group information if applicable
        if (sound != nullptr) {
            int groupIdx = sound->getGroupIndex();
            if (groupIdx >= 0 && groupIdx < owner->processor.getSampleManager().getNumGroups()) {
                sampleName += " [G" + juce::String(groupIdx + 1) + "]";
            }
        }

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

        // Set initial value
        float probability = owner->processor.getSampleManager().getSampleProbability(row);
        slider->setValue(probability, juce::dontSendNotification);

        // Connect slider value change to the ownerList
        slider->onValueChange = [this, owner, row]() {
            double value = slider->getValue();
            owner->handleSliderValueChanged(row, value);
        };

        // Set property to identify the slider for event handling
        slider->getProperties().set("slider", true);

        // Create icons with direct BinaryData pointers
        editIcon = std::make_unique<Icon>(BinaryData::pencil_svg, BinaryData::pencil_svgSize, 16.0f);
        onsetIcon = std::make_unique<Icon>(BinaryData::threelines_svg, BinaryData::threelines_svgSize, 16.0f);
        deleteIcon = std::make_unique<Icon>(BinaryData::delete_svg, BinaryData::delete_svgSize, 16.0f);
        reverbIcon = std::make_unique<TextIcon>("R", 16.0f);

        // Set icon colors
        editIcon->setNormalColour(juce::Colours::lightgrey);
        onsetIcon->setNormalColour(juce::Colours::lightgrey);
        deleteIcon->setNormalColour(juce::Colours::lightgrey);
        reverbIcon->setNormalColour(juce::Colours::lightgrey);

        // Set tooltips
        editIcon->setTooltip("Edit sample");
        onsetIcon->setTooltip(
                "Toggle onset randomization - each trigger will randomize the start position based on onset in the edit view.");
        deleteIcon->setTooltip("Delete sample");
        reverbIcon->setTooltip("Allow reverb for this sample.");
        slider->setTooltip("Sample probability");

        editIcon->onClicked = [owner, row]() {
            if (owner->onSampleDetailRequested)
                owner->onSampleDetailRequested(row);
        };

        onsetIcon->onClicked = [owner, row]() {
            owner->toggleOnsetRandomization(row);
        };

        deleteIcon->onClicked = [owner, row]() {
            owner->processor.getSampleManager().removeSamples(row, row);
            owner->updateContent();
        };

        reverbIcon->onClicked = [owner, row]() {
            owner->toggleReverbForSample(row);
        };

        // Configure onset icon state
        if (sound != nullptr) {
            bool hasOnsetMarkers = !sound->getOnsetMarkers().empty();
            bool isRandomizationEnabled = sound->isOnsetRandomizationEnabled();

            onsetIcon->setEnabled(hasOnsetMarkers);
            if (hasOnsetMarkers && isRandomizationEnabled) {
                onsetIcon->setActive(true, juce::Colour(0xff52bfd9));
            }

            // Configure reverb icon state
            if (sound->isReverbEnabled()) {
                reverbIcon->setActive(true, juce::Colour(0xff52bfd9));
            }
        }

        // Add all components
        addAndMakeVisible(slider.get());
        addAndMakeVisible(editIcon.get());
        addAndMakeVisible(onsetIcon.get());
        addAndMakeVisible(deleteIcon.get());
        addAndMakeVisible(reverbIcon.get());

        // Setup rate icons
        setupRateIcon(rate1_2Icon, "1/2", Params::RATE_1_2);
        setupRateIcon(rate1_4Icon, "1/4", Params::RATE_1_4);
        setupRateIcon(rate1_8Icon, "1/8", Params::RATE_1_8);
        setupRateIcon(rate1_16Icon, "1/16", Params::RATE_1_16);
    }

    void paint(juce::Graphics &g) override {
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(14.0f)));

        // Calculate the space needed for all controls
        int iconSize = 16;
        int rateIconWidth = 27;
        int padding = 8;
        int sliderSize = 16;
        
        // Calculate the space needed for all controls on the right
        int controlsWidth = 4 * rateIconWidth + // 4 rate icons
                          4 * iconSize +      // 4 icons (edit, onset, delete, reverb)
                          sliderSize +        // slider
                          9 * padding;        // padding between elements
                          
        // Calculate text area width dynamically based on available space
        int textAreaWidth = getWidth() - controlsWidth - 8; // Additional 8px margin
        
        // Ensure text area doesn't go below minimum width
        textAreaWidth = std::max(textAreaWidth, 50);

        // Draw the sample name with calculated width
        g.drawText(sampleName,
                   4,
                   0,
                   textAreaWidth,
                   getHeight(),
                   juce::Justification::centredLeft);
    }

    void resized() override {
        // Position slider first, followed by icons on the right
        int iconSize = 16;
        int rateIconWidth = 27; // Wider width for rate icons
        int padding = 4; // Reduced padding to ensure everything fits
        int sliderSize = 16; // Slightly larger for usability

        // Calculate total available width
        int availableWidth = getWidth();
        
        // Calculate the width needed for all controls
        int controlsWidth = 4 * rateIconWidth + // 4 rate icons
                            4 * iconSize +      // 4 icons (edit, onset, delete, reverb)
                            sliderSize +        // slider
                            9 * padding;        // padding between elements
        
        // Ensure minimum text area for sample name
        int minTextWidth = 100;
        
        // If we don't have enough space, reduce padding to ensure everything fits
        if (availableWidth < controlsWidth + minTextWidth) {
            padding = std::max(2, (availableWidth - minTextWidth - (4 * rateIconWidth + 4 * iconSize + sliderSize)) / 9);
        }

        // Start position for right-aligned elements
        int x = availableWidth - iconSize - padding;

        deleteIcon->setBounds(x, (getHeight() - iconSize) / 2, iconSize, iconSize);

        x -= iconSize + padding;
        editIcon->setBounds(x, (getHeight() - iconSize) / 2, iconSize, iconSize);

        x -= iconSize + padding;
        onsetIcon->setBounds(x, (getHeight() - iconSize) / 2, iconSize, iconSize);

        x -= iconSize + padding;
        reverbIcon->setBounds(x, (getHeight() - iconSize) / 2, iconSize, iconSize);

        // Position slider before the icons
        x -= sliderSize + padding;
        slider->setBounds(x, (getHeight() - sliderSize) / 2, sliderSize, sliderSize);

        // Position rate icons before the slider, ensuring each is fully visible
        x -= rateIconWidth + padding;
        rate1_16Icon->setBounds(x, (getHeight() - iconSize) / 2, rateIconWidth, iconSize);

        x -= rateIconWidth + padding;
        rate1_8Icon->setBounds(x, (getHeight() - iconSize) / 2, rateIconWidth, iconSize);

        x -= rateIconWidth + padding;
        rate1_4Icon->setBounds(x, (getHeight() - iconSize) / 2, rateIconWidth, iconSize);

        x -= rateIconWidth + padding;
        rate1_2Icon->setBounds(x, (getHeight() - iconSize) / 2, rateIconWidth, iconSize);
    }

    void mouseDown(const juce::MouseEvent &e) override {
        // Only forward mouse events if not clicking on an icon
        if (!e.eventComponent->getProperties().contains("icon")) {
            // Forward the event to the parent component for selection handling
            if (juce::Component *parent = getParentComponent())
                parent->mouseDown(e.getEventRelativeTo(parent));
        }
    }

    void mouseUp(const juce::MouseEvent &e) override {
        // Only forward mouse events if not clicking on an icon
        if (!e.eventComponent->getProperties().contains("icon")) {
            // Forward the event to the parent component
            if (juce::Component *parent = getParentComponent())
                parent->mouseUp(e.getEventRelativeTo(parent));
        }
    }

    void mouseDrag(const juce::MouseEvent &e) override {
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

private:
    SampleList *ownerList;
    int rowNumber;
    juce::String sampleName;

    std::unique_ptr<juce::Slider> slider;
    std::unique_ptr<Icon> editIcon;
    std::unique_ptr<Icon> onsetIcon;
    std::unique_ptr<Icon> deleteIcon;
    std::unique_ptr<TextIcon> reverbIcon;

    // Rate icons
    std::unique_ptr<TextIcon> rate1_2Icon;
    std::unique_ptr<TextIcon> rate1_4Icon;
    std::unique_ptr<TextIcon> rate1_8Icon;
    std::unique_ptr<TextIcon> rate1_16Icon;

    void setupRateIcon(std::unique_ptr<TextIcon> &icon, const juce::String &text, Params::RateOption rate) {
        icon = std::make_unique<TextIcon>(text, 27.0f, 16.0f);
        icon->setNormalColour(juce::Colours::lightgrey);
        icon->setTooltip("Toggle " + text + " rate");

        // Set initial state
        bool isEnabled = ownerList->processor.getSampleManager().isSampleRateEnabled(rowNumber, rate);
        if (isEnabled)
            icon->setActive(true, juce::Colour(0xff52bfd9));

        // Add click handler
        icon->onClicked = [this, rate]() {
            bool currentState = ownerList->processor.getSampleManager().isSampleRateEnabled(rowNumber, rate);
            ownerList->processor.getSampleManager().setSampleRateEnabled(rowNumber, rate, !currentState);

            // Update icon state
            if (auto *icon = getRateIcon(rate)) {
                if (!currentState)
                    icon->setActive(true, juce::Colour(0xff52bfd9));
                else
                    icon->setActive(false);
            }
        };

        addAndMakeVisible(icon.get());
    }

    TextIcon *getRateIcon(Params::RateOption rate) {
        switch (rate) {
            case Params::RATE_1_2:
                return rate1_2Icon.get();
            case Params::RATE_1_4:
                return rate1_4Icon.get();
            case Params::RATE_1_8:
                return rate1_8Icon.get();
            case Params::RATE_1_16:
                return rate1_16Icon.get();
            default:
                return nullptr;
        }
    }
};