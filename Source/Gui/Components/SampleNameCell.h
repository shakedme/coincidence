
#include <juce_audio_utils/juce_audio_utils.h>
#include "SampleList.h"
#include "../../Audio/Sampler/Sampler.h"

// A complete cell component that includes both sample name and icons
class SampleNameCellComponent : public juce::Component
{
public:
    SampleNameCellComponent(SampleList* owner, int row, SamplerSound* sound)
        : ownerList(owner), rowNumber(row)
    {
        // Get the sample name
        sampleName = owner->processor.getSampleManager().getSampleName(row);

        // Add group information if applicable
        if (sound != nullptr)
        {
            int groupIdx = sound->getGroupIndex();
            if (groupIdx >= 0 && groupIdx < owner->processor.getSampleManager().getNumGroups())
            {
                if (const SampleManager::Group* group = owner->processor.getSampleManager().getGroup(groupIdx))
                {
                    sampleName += " [G" + juce::String(groupIdx + 1) + "]";
                }
            }
        }

        // Create icons with direct BinaryData pointers
        editIcon = std::make_unique<Icon>(BinaryData::pencil_svg, BinaryData::pencil_svgSize, 16.0f);
        onsetIcon = std::make_unique<Icon>(BinaryData::threelines_svg, BinaryData::threelines_svgSize, 16.0f);
        deleteIcon = std::make_unique<Icon>(BinaryData::delete_svg, BinaryData::delete_svgSize, 16.0f);

        // Set icon colors
        editIcon->setNormalColour(juce::Colours::lightgrey);
        onsetIcon->setNormalColour(juce::Colours::lightgrey);
        deleteIcon->setNormalColour(juce::Colours::lightgrey);

        // Set tooltips
        editIcon->setTooltip("Edit sample");
        onsetIcon->setTooltip("Toggle onset randomization");
        deleteIcon->setTooltip("Delete sample");

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

        // Configure onset icon state
        if (sound != nullptr)
        {
            bool hasOnsetMarkers = !sound->getOnsetMarkers().empty();
            bool isRandomizationEnabled = sound->isOnsetRandomizationEnabled();

            onsetIcon->setEnabled(hasOnsetMarkers);
            if (hasOnsetMarkers && isRandomizationEnabled)
            {
                onsetIcon->setActive(true, juce::Colour(0xff52bfd9));
            }
        }

        // Add icons to component
        addAndMakeVisible(editIcon.get());
        addAndMakeVisible(onsetIcon.get());
        addAndMakeVisible(deleteIcon.get());
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(14.0f)));

        // Draw the sample name (adjusted to leave space for icons)
        g.drawText(sampleName,
                   4,
                   0,
                   getWidth() - 100,
                   getHeight(),
                   juce::Justification::centredLeft);
    }

    void resized() override
    {
        // Position icons at the right side
        int iconSize = 16;
        int padding = 8;
        int x = getWidth() - iconSize - padding;

        deleteIcon->setBounds(x, (getHeight() - iconSize) / 2, iconSize, iconSize);

        x -= iconSize + padding;
        editIcon->setBounds(x, (getHeight() - iconSize) / 2, iconSize, iconSize);

        x -= iconSize + padding;
        onsetIcon->setBounds(x, (getHeight() - iconSize) / 2, iconSize, iconSize);
    }

private:
    SampleList* ownerList;
    int rowNumber;
    juce::String sampleName;

    std::unique_ptr<Icon> editIcon;
    std::unique_ptr<Icon> onsetIcon;
    std::unique_ptr<Icon> deleteIcon;
};