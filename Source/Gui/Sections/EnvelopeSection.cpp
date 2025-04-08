#include "EnvelopeSection.h"
#include "../Components/Envelope/EnvelopeComponent.h"
#include <memory>

EnvelopeSection::EnvelopeSection(PluginEditor &editor, PluginProcessor &processor)
        : BaseSectionComponent(editor, processor, "", juce::Colour(0xff8a6e9e)) {

    lfoTabs = std::make_unique<EnvelopeTabs>(juce::TabbedButtonBar::TabsAtTop);
    lfoTabs->setOutline(0);
    lfoTabs->setTabBarDepth(30);
    addAndMakeVisible(lfoTabs.get());

    addLFOButton.setButtonText("+");
    addLFOButton.onClick = [this] { onAddLFOClicked(); };
    addAndMakeVisible(addLFOButton);

    waveformComponent.setBackgroundColour(juce::Colours::transparentBlack);
    waveformComponent.setWaveformAlpha(0.3f);
    waveformComponent.setWaveformColour(juce::Colour(0xff52bfd9));
    waveformComponent.setWaveformScaleFactor(1.0f);
    addAndMakeVisible(waveformComponent);

    createLFOComponents();

    juce::Timer::callAfterDelay(500, [this]() {
        updateTimeRangeFromRate(lfoComponents[0]->getRateEnum());
    });
}

void EnvelopeSection::createLFOComponents() {
    for (int i = 0; i < 3; ++i) {
        addLFOComponent(i);
    }

    lfoTabs->onTabChanged = [this](int tabIndex) {
        for (auto &pair: lfoComponents) {
            pair.second->setVisible(false);
        }
        updateTimeRangeFromRate(lfoComponents[tabIndex]->getRateEnum());
        lfoComponents[tabIndex]->setVisible(true);
    };

    lfoTabs->setCurrentTabIndex(0);
    lfoComponents[0]->setVisible(true);
}

void EnvelopeSection::addLFOComponent(int index) {
    auto component = std::make_unique<EnvelopeComponent>(processor);
    component->onRateChanged = [this](Models::LFORate rate) { updateTimeRangeFromRate(rate); };
    lfoComponents.insert(std::make_pair(index, std::move(component)));
    lfoTabs->addTab("LFO " + juce::String(index + 1), juce::Colours::transparentBlack, nullptr, false);
    addAndMakeVisible(lfoComponents[index].get());
    lfoComponents[index]->setVisible(false);
}

void EnvelopeSection::onAddLFOClicked() {
    if (lfoComponents.size() >= 8) {
        return;
    }
    int newIndex = static_cast<int>(lfoComponents.size());
    addLFOComponent(newIndex);
    lfoTabs->setCurrentTabIndex(newIndex);
    resized();
}

void EnvelopeSection::paint(juce::Graphics &g) {
    BaseSectionComponent::paint(g);
}

void EnvelopeSection::resized() {
    auto lfoArea = getLocalBounds();

    const int tabsHeight = 30;
    lfoTabs->setBounds(lfoArea.removeFromTop(tabsHeight));
    int realTabsWidth = 0;
    for (int i = 0; i < lfoTabs->getNumTabs(); ++i) {
        realTabsWidth += lfoTabs->getTabbedButtonBar().getTabButton(i)->getWidth();
    }
    addLFOButton.setBounds(realTabsWidth + 10, 2, tabsHeight - 4, tabsHeight - 4);

    auto lfoComponentArea = lfoArea.reduced(10, 10);
    for (auto &pair: lfoComponents) {
        pair.second->setBounds(lfoComponentArea);
    }

    lfoComponentArea.removeFromBottom(65);
    waveformComponent.setBounds(lfoComponentArea);
}

void EnvelopeSection::updateTimeRangeFromRate(Models::LFORate newRate) {
    setTimeRange(calculateTimeRangeInSeconds(newRate));
}

void EnvelopeSection::setSampleRate(float newSampleRate) {
    waveformComponent.setSampleRate(newSampleRate);
}

void EnvelopeSection::setTimeRange(float seconds) {
    waveformComponent.setTimeRange(seconds);
}

void EnvelopeSection::pushAudioBuffer(const float *audioData, int numSamples) {
    waveformComponent.pushAudioBuffer(audioData, numSamples);
}

float EnvelopeSection::calculateTimeRangeInSeconds(Models::LFORate newRate) const {
    double bpm = processor.getTimingManager().getBpm();
    const double beatsPerSecond = bpm / 60.0;

    float timeRangeInSeconds = 1.0f;

    switch (newRate) {
        case Models::LFORate::TwoWhole:
            timeRangeInSeconds = static_cast<float>(8.0 / beatsPerSecond);
            break;
        case Models::LFORate::Whole:
            timeRangeInSeconds = static_cast<float>(4.0 / beatsPerSecond);
            break;
        case Models::LFORate::Half:
            timeRangeInSeconds = static_cast<float>(2.0 / beatsPerSecond);
            break;
        case Models::LFORate::Quarter:
            timeRangeInSeconds = static_cast<float>(1.0 / beatsPerSecond);
            break;
        case Models::LFORate::Eighth:
            timeRangeInSeconds = static_cast<float>(0.5 / beatsPerSecond);
            break;
        case Models::LFORate::Sixteenth:
            timeRangeInSeconds = static_cast<float>(0.25 / beatsPerSecond);
            break;
        case Models::LFORate::ThirtySecond:
            timeRangeInSeconds = static_cast<float>(0.125 / beatsPerSecond);
            break;
    }

    return timeRangeInSeconds;
}