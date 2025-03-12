#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "SampleList.h"
#include "../../Audio/Sampler/Sampler.h"
#include "Icon.h"

// A complete cell component that includes both sample name, probability slider and icons
class SampleRow : public juce::Component {
public:
    SampleRow(SampleList *owner, int row, SamplerSound *sound);

    void paint(juce::Graphics &g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void updateContent(SampleList *owner, int row, SamplerSound *sound);
private:
    SampleList *ownerList;
    int rowNumber;
    juce::String sampleName;

    std::unique_ptr<juce::Slider> slider;
    std::unique_ptr<Icon> editIcon;
    std::unique_ptr<Icon> onsetIcon;
    std::unique_ptr<Icon> deleteIcon;

    // Rate icons
    std::unique_ptr<TextIcon> rate1_1Icon;
    std::unique_ptr<TextIcon> rate1_2Icon;
    std::unique_ptr<TextIcon> rate1_4Icon;
    std::unique_ptr<TextIcon> rate1_8Icon;
    std::unique_ptr<TextIcon> rate1_16Icon;
    std::unique_ptr<TextIcon> rate1_32Icon;

    int rateIconWidth = 30;

    void setupRateIcon(std::unique_ptr<TextIcon> &icon, const juce::String &text, Params::RateOption rate);
    void updateRateIcon(Params::RateOption rate);
    TextIcon *getRateIcon(Params::RateOption rate);
};