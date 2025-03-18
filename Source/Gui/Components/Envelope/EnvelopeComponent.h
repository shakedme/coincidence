#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include "../../../Shared/Envelope/EnvelopeParameterMapper.h"
#include "../../../Shared/Envelope/EnvelopeParameterTypes.h"
#include "../../../Shared/Envelope/EnvelopePoint.h"
#include "../../../Shared/TimingManager.h"
#include "../WaveformComponent.h"
#include "EnvelopePresetGenerator.h"
#include "EnvelopePointManager.h"
#include "EnvelopeRenderer.h"
#include "EnvelopeShapeButton.h"

class EnvelopeComponent : public juce::Component, private juce::Timer {
public:

    explicit EnvelopeComponent(
            TimingManager &tm,
            EnvelopeParams::ParameterType type = EnvelopeParams::ParameterType::Amplitude);

    ~EnvelopeComponent() override;

    void paint(juce::Graphics &) override;

    void resized() override;

    // Mouse interaction
    void mouseDown(const juce::MouseEvent &e) override;

    void mouseDrag(const juce::MouseEvent &e) override;

    void mouseUp(const juce::MouseEvent &e) override;

    void mouseDoubleClick(const juce::MouseEvent &e) override;

    bool keyPressed(const juce::KeyPress &key) override;

    // Set the sample rate for audio visualization
    void setSampleRate(float newSampleRate);

    // Set time range in seconds (for scaling the waveform)
    void setTimeRange(float seconds);

    // Add audio samples for visualization
    void pushAudioBuffer(const float *audioData, int numSamples);

    // Parameter mapping
    void setParameterRange(float min, float max, bool exponential = false);

    void setParameterType(EnvelopeParams::ParameterType type);

    // Get the current envelope value
    float getCurrentValue() const;

    // Set envelope rate
    void setRate(float newRate);

    void setSettings(EnvelopeParams::ParameterSettings settings) {
        parameterMapper.setSettings(settings);
    }

    // Callbacks for parameter change notifications
    std::function<void(const std::vector<std::unique_ptr<EnvelopePoint>> &points)> onPointsChanged;
    std::function<void(float)> onRateChanged;

    enum class Rate {
        TwoWhole = 0,
        Whole,
        Half,
        Quarter,
        Eighth,
        Sixteenth,
        ThirtySecond
    };

private:
    // Handler methods for utility class callbacks
    void handlePointsChanged();

    void timerCallback() override;

    void updateTimeRangeFromRate();

    void resizeControls(int width, int topPadding = 5);

    void updateRateFromComboBox();

    void setCurrentPresetShape(EnvelopePresetGenerator::PresetShape shape);

    void setupRateUI();

    void setupPresetsUI();

    void handlePresetButtonClick(EnvelopePresetGenerator::PresetShape shape);

    [[nodiscard]] float calculateTimeRangeInSeconds(double bpm) const;

    int removeFromTop = 65;

    std::unique_ptr<juce::ComboBox> rateComboBox;
    Rate currentRateEnum = Rate::Quarter;
    float currentRate = 1.0f;

    // Selection area
    bool isCreatingSelectionArea = false;
    juce::Point<float> selectionStart;
    juce::Rectangle<float> selectionArea;

    // Point dragging tracking
    int draggedPointIndex = -1;

    // Curve editing
    int curveEditingSegment = -1;
    float initialCurvature = 0.0f;
    juce::Point<float> curveEditStartPos;

    // Waveform component for visualization
    WaveformComponent waveformComponent;

    // Parameter mapper
    EnvelopeParameterMapper parameterMapper;

    TimingManager &timingManager;

    // Utility classes
    EnvelopePointManager pointManager;
    EnvelopeRenderer renderer;

    // Preset shape buttons
    std::vector<std::unique_ptr<EnvelopeShapeButton>> presetButtons;
    EnvelopePresetGenerator::PresetShape currentPresetShape = EnvelopePresetGenerator::PresetShape::Sine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeComponent)
}; 