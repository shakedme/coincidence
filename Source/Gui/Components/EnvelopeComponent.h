#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include "../../Audio/Envelope/EnvelopeParameterMapper.h"
#include "../../Audio/Envelope/EnvelopeParameterTypes.h"
#include "../../Shared/TimingManager.h"
#include "WaveformComponent.h"

class EnvelopeComponent : public juce::Component, private juce::Timer {
public:
    // Rate enum for time signature divisions
    enum class Rate
    {
        TwoWhole = 0,
        Whole,
        Half,
        Quarter,     // 1/4 note - 1 beat
        Eighth,      // 1/8 note - 1/2 beat
        Sixteenth,   // 1/16 note - 1/4 beat
        ThirtySecond // 1/32 note - 1/8 beat
    };

    explicit EnvelopeComponent(
            TimingManager& tm,
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

    // Get the waveform component
    WaveformComponent* getWaveformComponent() { return &waveformComponent; }

    // Callbacks for parameter change notifications
    std::function<void()> onPointsChanged;
    std::function<void(float)> onRateChanged;

    // Parameter mapping
    void setParameterRange(float min, float max, bool exponential = false);

    void setParameterType(EnvelopeParams::ParameterType type);

    void setWaveformScaleFactor(float scale);

    EnvelopeParams::ParameterType getParameterType() const;

    // Get the current envelope value
    float getCurrentValue() const;

    // Update the envelope time
    void updateTime(float deltaTime);

    // Set envelope rate
    void setRate(float newRate);

    float getRate() const { return parameterMapper.getRate(); }

    // Access envelope points
    const std::vector<std::unique_ptr<EnvelopePoint>> &getPoints() const { return points; }

    // Set the rate using the Rate enum
    void setRateFromEnum(Rate rate);
    
    // Get the current Rate enum value
    Rate getCurrentRateEnum() const;

private:
    // Draw helper methods
    void drawGrid(juce::Graphics &g);

    void drawEnvelopeLine(juce::Graphics &g);

    void drawPoints(juce::Graphics &g);

    void drawSelectionArea(juce::Graphics &g);

    void drawPositionMarker(juce::Graphics &g);

    // Position helpers
    juce::Point<float> getPointScreenPosition(const EnvelopePoint &point) const;

    int findClosestSegmentIndex(const juce::Point<float> &clickPos) const;

    float distanceToLineSegment(const juce::Point<float> &p, const juce::Point<float> &v,
                                const juce::Point<float> &w) const;

    float distanceToCurve(const juce::Point<float> &point, const juce::Point<float> &start,
                          const juce::Point<float> &end, float curvature) const;

    // Selection methods
    void selectPointsInArea();

    int getSelectedPointsCount() const;

    void timerCallback() override;

    // Point change notification
    void notifyPointsChanged();

    // Setup and update rate UI
    void setupRateUI();
    void updateTimeRangeFromRate();
    void updateRateFromComboBox();

    // Rate UI components
    std::unique_ptr<juce::ComboBox> rateComboBox;
    std::unique_ptr<juce::Label> rateLabel;
    Rate currentRateEnum = Rate::Quarter;

    // Envelope data
    std::vector<std::unique_ptr<EnvelopePoint>> points;
    EnvelopePoint *pointDragging = nullptr;
    bool isDraggingSelectedPoints = false;
    juce::Point<float> lastDragPosition;

    // Envelope parameters
    static constexpr float pointRadius = 6.0f;
    int horizontalDivisions = 10;
    int verticalDivisions = 4;

    // Curve editing
    int curveEditingSegment = -1;
    float initialCurvature = 0.0f;
    juce::Point<float> curveEditStartPos;

    // Selection area
    bool isCreatingSelectionArea = false;
    juce::Point<float> selectionStart;
    juce::Rectangle<float> selectionArea;

    // Waveform component for visualization
    WaveformComponent waveformComponent;

    // Parameter mapper
    EnvelopeParameterMapper parameterMapper;

    TimingManager& timingManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeComponent)
}; 