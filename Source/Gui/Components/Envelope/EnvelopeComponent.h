#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include "../../../Shared/Envelope/EnvelopeParameterMapper.h"
#include "../../../Shared/Envelope/EnvelopeParameterTypes.h"
#include "../../../Shared/Envelope/EnvelopePoint.h"
#include "EnvelopePresetGenerator.h"
#include "EnvelopeGridSystem.h"
#include "EnvelopePointManager.h"
#include "EnvelopeRenderer.h"
#include "EnvelopeUIControlsManager.h"
#include "../../../Shared/TimingManager.h"
#include "../WaveformComponent.h"

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

    void setWaveformScaleFactor(float scale);

    // Get the current envelope value
    float getCurrentValue() const;

    // Set envelope rate
    void setRate(float newRate);

    // Access envelope points
    const std::vector<std::unique_ptr<EnvelopePoint>> &getPoints() const;

    // Preset shape methods
    void applyPresetShape(EnvelopePresetGenerator::PresetShape shape);

    // Snap-to-grid functionality
    void setSnapToGrid(bool shouldSnap);

    void setSettings(EnvelopeParams::ParameterSettings settings) {
        parameterMapper.setSettings(settings);
    }

    // Callbacks for parameter change notifications
    std::function<void(const std::vector<std::unique_ptr<EnvelopePoint>> &points)> onPointsChanged;
    std::function<void(float)> onRateChanged;

private:
    // Handler methods for utility class callbacks
    void handlePointsChanged();

    void handlePresetShapeChanged(EnvelopePresetGenerator::PresetShape shape);

    void handleSnapToGridChanged(bool enabled);

    void handleRateChanged(float rate);

    void timerCallback() override;

    void updateTimeRangeFromRate();

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
    EnvelopeGridSystem gridSystem;
    EnvelopePointManager pointManager;
    EnvelopeRenderer renderer;
    EnvelopeUIControlsManager uiControlsManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeComponent)
}; 