#pragma once

#include "../Components/Envelope/EnvelopeTabs.h"
#include "BaseSection.h"
#include <map>
#include "../Components/WaveformComponent.h"
#include "../../Shared/Models.h"

// Forward declarations
class PluginEditor;

class PluginProcessor;

class EnvelopeComponent;

class EnvelopeSection : public BaseSectionComponent {
public:
    EnvelopeSection(PluginEditor &editor, PluginProcessor &processor);

    ~EnvelopeSection() override = default;

    void paint(juce::Graphics &g) override;

    void resized() override;

    void setSampleRate(float newSampleRate);

    void setTimeRange(float seconds);

    void pushAudioBuffer(const float *audioData, int numSamples);

    std::shared_ptr<EnvelopeComponent> getLFOComponent(int index) {
        return lfoComponents[index];
    }

private:
    juce::TextButton addLFOButton;
    std::unique_ptr<EnvelopeTabs> lfoTabs;
    WaveformComponent waveformComponent;
    std::unordered_map<int, std::shared_ptr<EnvelopeComponent>> lfoComponents;

    void createLFOComponents();

    void addLFOComponent(int index);

    void onAddLFOClicked();

    [[nodiscard]] float calculateTimeRangeInSeconds(Models::LFORate newRate) const;

    void updateTimeRangeFromRate(Models::LFORate newRate);


}; 