#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include "EnvelopeParameterMapper.h"
#include "EnvelopePoint.h"
#include "../../../Shared/TimingManager.h"
#include "EnvelopePresetGenerator.h"
#include "EnvelopePointManager.h"
#include "EnvelopeRenderer.h"
#include "EnvelopeShapeButton.h"

class EnvelopeComponent : public juce::Component, private juce::Timer {
public:

    explicit EnvelopeComponent(PluginProcessor &p);

    ~EnvelopeComponent() override;

    void paint(juce::Graphics &) override;

    void resized() override;

    void mouseDown(const juce::MouseEvent &e) override;

    void mouseDrag(const juce::MouseEvent &e) override;

    void mouseUp(const juce::MouseEvent &e) override;

    void mouseDoubleClick(const juce::MouseEvent &e) override;

    bool keyPressed(const juce::KeyPress &key) override;

    float getRate() { return currentRate; }

    Models::LFORate getRateEnum() { return currentRateEnum; }

    [[nodiscard]] const std::vector<std::unique_ptr<EnvelopePoint>> &getPoints();

    std::function<void(Models::LFORate rate)> onRateChanged;

private:
    void handlePointsChanged();

    void timerCallback() override;

    void resizeControls(int width, int topPadding = 5);

    void updateRateFromComboBox();

    void setCurrentPresetShape(EnvelopePresetGenerator::PresetShape shape);

    void setupRateUI();

    void setupPresetsUI();

    void handlePresetButtonClick(EnvelopePresetGenerator::PresetShape shape);

    int removeFromTop = 65;

    std::unique_ptr<juce::ComboBox> rateComboBox;
    Models::LFORate currentRateEnum = Models::LFORate::Quarter;
    float currentRate = 1.0f;

    bool isCreatingSelectionArea = false;
    juce::Point<float> selectionStart;
    juce::Rectangle<float> selectionArea;

    int draggedPointIndex = -1;
    int curveEditingSegment = -1;
    float initialCurvature = 0.0f;
    juce::Point<float> curveEditStartPos;

    PluginProcessor &processor;

    EnvelopePointManager pointManager;
    EnvelopeRenderer renderer;

    std::vector<std::unique_ptr<EnvelopeShapeButton>> presetButtons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeComponent)
}; 