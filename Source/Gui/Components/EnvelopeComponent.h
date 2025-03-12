#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include "../../Audio/Envelope/EnvelopeParameterMapper.h"
#include "../../Audio/Envelope/EnvelopeParameterTypes.h"
#include "../../Audio/Shared/TimingManager.h"
#include "../../Audio/Util/AudioBufferQueue.h"

class EnvelopeComponent : public juce::Component, private juce::Timer {
public:
    explicit EnvelopeComponent(EnvelopeParams::ParameterType type = EnvelopeParams::ParameterType::Amplitude);

    ~EnvelopeComponent() override;

    void paint(juce::Graphics &) override;

    void resized() override;

    // Mouse interaction
    void mouseDown(const juce::MouseEvent &e) override;

    void mouseDrag(const juce::MouseEvent &e) override;

    void mouseUp(const juce::MouseEvent &e) override;

    void mouseDoubleClick(const juce::MouseEvent &e) override;

    bool keyPressed(const juce::KeyPress &key) override;

    // Set the factor to scale the waveform by
    void setWaveformScaleFactor(float scale);

    // Set the sample rate for audio visualization
    void setSampleRate(float newSampleRate);

    // Set time range in seconds (for scaling the waveform)
    void setTimeRange(float seconds);

    // Add audio samples for visualization
    void pushAudioBuffer(const float *audioData, int numSamples);

    // Callbacks for parameter change notifications
    std::function<void()> onPointsChanged;
    std::function<void(float)> onRateChanged;

    // Parameter mapping
    void setParameterRange(float min, float max, bool exponential = false);

    void setParameterType(EnvelopeParams::ParameterType type);

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

    // Set the TimingManager to get transport position
    void setTimingManager(std::shared_ptr<TimingManager> manager) { timingManager = manager; }

private:
    // Draw helper methods
    void drawGrid(juce::Graphics &g);

    void drawEnvelopeLine(juce::Graphics &g);

    void drawPoints(juce::Graphics &g);

    void drawWaveform(juce::Graphics &g);

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

    // Waveform rendering
    void setupWaveformRendering();

    void updateWaveformCache();

    void timerCallback() override;

    // Point change notification
    void notifyPointsChanged();

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

    // Waveform visualization
    AudioBufferQueue *audioBufferQueue = nullptr;
    std::vector<float> waveformData;
    struct PeakData {
        float min = 0.0f;
        float max = 0.0f;
    };
    std::vector<PeakData> waveformPeaks;
    juce::Image waveformCache;
    std::atomic<bool> waveformNeedsRedraw{false};
    std::mutex waveformMutex;
    float sampleRate = 44100.0f;
    float timeRangeInSeconds = 1.0f;
    float waveformScaleFactor = 1.0f;
    juce::Colour waveformColour = juce::Colour(0xff52bfd9);

    // Parameter mapper
    EnvelopeParameterMapper parameterMapper;

    // Timing manager reference (for transport sync)
    std::shared_ptr<TimingManager> timingManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeComponent)
}; 