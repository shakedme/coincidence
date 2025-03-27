#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <atomic>
#include <memory>
#include "EnvelopeParameterTypes.h"

// Forward class declaration for envelope points
class EnvelopePoint;

// Class to map envelope parameters for real-time audio processing
class EnvelopeParameterMapper {
public:
    EnvelopeParameterMapper();

    ~EnvelopeParameterMapper();

    // Get current value (thread-safe)
    float getCurrentValue() const;

    // Get value at a specific position (0-1) in the envelope
    float getValueAtPosition(float position) const;

    // Set transport position
    void setTransportPosition(double ppqPosition);

    // Set the rate multiplier
    void setRate(float newRate);

    // Get the current rate
    float getRate() const { return rate; }

    void setParameterId(juce::Identifier paramId) { paramId = paramId; }

    // Set the value range for the parameter
    void setParameterRange(float min, float max, bool isExponential);

    // Set bipolar mode (whether param can go negative)
    void setBipolar(bool isBipolar);

    // Set the envelope points (thread-safe)
    void setPoints(const std::vector<std::unique_ptr<EnvelopePoint>> &newPoints);

    void setSettings(EnvelopeParams::ParameterSettings settings) { settings = settings; }

    // Get a copy of the current envelope points
    std::vector<std::unique_ptr<EnvelopePoint>> getPointsCopy() const;

    // Reset to default points
    void clearPoints();

    // Get the parameter type
    juce::Identifier getParameterId() const { return paramId; }

private:
    // Lock-free buffer structure for points
    struct PointBuffer {
        std::vector<std::unique_ptr<EnvelopePoint>> points;
    };

    // Create a deep copy of a point buffer
    std::unique_ptr<PointBuffer> createPointBufferCopy(const PointBuffer *source) const;

    // Add default points to a buffer
    void addDefaultPointsToBuffer(PointBuffer *buffer) const;

    // Interpolate value between points
    float interpolateValue(float time) const;

    // Map normalized value to parameter range
    float mapToParameterRange(float normalizedValue) const;

    juce::Identifier paramId;

    // Parameter settings
    EnvelopeParams::ParameterSettings settings;

    // Rate multiplier (higher values = faster envelope)
    float rate = 1.0f;

    // Whether to use transport position for sync
    bool useTransportSync = true;

    // Current position info
    double currentPpqPosition = -1.0;
    float currentTime = 0.0f;

    // Active point buffer (read by audio thread)
    std::atomic<PointBuffer *> activePointBuffer;

    // Edit buffer (written by UI thread)
    PointBuffer *editBuffer;
}; 