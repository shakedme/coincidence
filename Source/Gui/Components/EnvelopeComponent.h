#pragma once

#include "../../Audio/Util/AudioBufferQueue.h"
#include <juce_audio_utils/juce_audio_utils.h>

class EnvelopePoint {
public:
    EnvelopePoint(float x, float y) : position(x, y) {}

    juce::Point<float> position;
    bool selected = false;
    float curvature = 0.0f; // 0.0 = straight line, -1.0 to 1.0 = curved
};

class EnvelopeComponent : public juce::Component, private juce::Timer {
public:
    EnvelopeComponent();

    ~EnvelopeComponent() override;

    void resized() override;

    void paint(juce::Graphics &g) override;

    void mouseDown(const juce::MouseEvent &e) override;

    void mouseDrag(const juce::MouseEvent &e) override;

    void mouseUp(const juce::MouseEvent &) override;

    void mouseDoubleClick(const juce::MouseEvent &e) override;

    void setGridDivisions(int vertical, int horizontal);

    // Thread-safe method to update audio buffer data from audio thread
    void pushAudioBuffer(const float *audioData, int numSamples);

    // Set the waveform display parameters
    void setWaveformColour(juce::Colour colour);

    void setWaveformScaleFactor(float scale);

    // Set the sample rate for the waveform display
    void setSampleRate(float newSampleRate);

    // Set the time range displayed in the waveform (in seconds)
    void setTimeRange(float seconds);

    // Set the direction of the waveform scrolling
    void setScrollDirection(bool leftToRight);

    // Toggle the scroll direction
    void toggleScrollDirection();

    // Get the current scroll direction
    bool isScrollingLeftToRight() const;

private:
    std::vector<std::unique_ptr<EnvelopePoint>> points;
    EnvelopePoint *pointDragging = nullptr;
    int curveEditingSegment = -1;
    juce::Point<float> curveEditStartPos;
    float initialCurvature = 0.0f;

    const float pointRadius = 6.0f;
    int verticalDivisions = 10;
    int horizontalDivisions = 8;

    // Structure to store min-max peak values for each pixel column
    struct WaveformPeak {
        float min = 0.0f;
        float max = 0.0f;
    };

    // Waveform visualization members
    AudioBufferQueue *audioBufferQueue = nullptr;
    juce::Image waveformCache;
    std::atomic<bool> waveformNeedsRedraw{false};
    std::vector<float> waveformData;
    std::vector<WaveformPeak> waveformPeaks; // Min-max peak data for high-resolution display
    juce::Colour waveformColour{juce::Colour(0xff3a5c8b)}; // Default to a nice blue
    float waveformScaleFactor = 1.0f;
    std::mutex waveformMutex; // Protects waveform cache updates
    float timeRangeInSeconds = 2.0f; // Default to 2 seconds
    int writePosition = 0; // Current write position for scrolling waveform
    float sampleRate = 44100.0f; // Default sample rate
    bool waveformScrollLeftToRight = true; // Direction of scrolling

    void timerCallback() override;

    void setupWaveformRendering();

    void updateWaveformCache();

    void drawWaveform(juce::Graphics &g);

    void drawGrid(juce::Graphics &g);

    void drawEnvelopeLine(juce::Graphics &g);

    void drawPoints(juce::Graphics &g);

    juce::Point<float> getPointScreenPosition(const EnvelopePoint &point) const;

    int findClosestSegmentIndex(const juce::Point<float> &clickPos) const;

    // Calculate distance from a point to a line segment
    float
    distanceToLineSegment(const juce::Point<float> &p, const juce::Point<float> &v, const juce::Point<float> &w) const;

    // Calculate distance from a point to a curved segment
    float distanceToCurve(const juce::Point<float> &point, const juce::Point<float> &start,
                          const juce::Point<float> &end, float curvature) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeComponent)
}; 