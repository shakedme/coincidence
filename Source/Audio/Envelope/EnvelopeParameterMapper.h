#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include "EnvelopeParameterMapper.h"
#include "EnvelopeParameterTypes.h"

class EnvelopePoint {
public:
    EnvelopePoint(float x, float y, bool _isEditable = true) : position(x, y) {
        isEditable = _isEditable;
    }

    juce::Point<float> position;
    bool selected = false;
    bool isEditable = true;
    float curvature = 0.0f; // 0.0 = straight line, -1.0 to 1.0 = curved
};

/**
 * Maps envelope control points to parameter values for various effect parameters
 */
class EnvelopeParameterMapper {
public:
    explicit EnvelopeParameterMapper(EnvelopeParams::ParameterType type = EnvelopeParams::ParameterType::Amplitude);

    ~EnvelopeParameterMapper();

    // Get the current parameter value based on time and envelope shape
    float getCurrentValue() const;

    // Get the current value at a specific position (0-1)
    float getValueAtPosition(float position) const;

    // Update internal time counter
    void updateTime(float deltaTime);

    // Set the transport position directly
    void setTransportPosition(double ppqPosition);

    // Set the rate at which the envelope cycles
    void setRate(float newRate);

    float getRate() const { return rate; }

    // Manage envelope settings
    void setParameterType(EnvelopeParams::ParameterType type);

    EnvelopeParams::ParameterType getParameterType() const { return parameterType; }

    void setParameterRange(float min, float max, bool isExponential = false);

    void setBipolar(bool isBipolar);

    // Manage envelope points - thread-safe methods
    void setPoints(const std::vector<std::unique_ptr<EnvelopePoint>> &newPoints);

    std::vector<std::unique_ptr<EnvelopePoint>> getPointsCopy() const;

    void clearPoints();

private:
    // Thread-safe point buffer structure
    struct PointBuffer {
        std::vector<std::unique_ptr<EnvelopePoint>> points;

        ~PointBuffer() = default;
    };

    // Create a new buffer with the same points (deep copy)
    std::unique_ptr<PointBuffer> createPointBufferCopy(const PointBuffer *source) const;

    // Add default points to a buffer
    void addDefaultPointsToBuffer(PointBuffer *buffer) const;

    // Internal helper methods
    void updateSettings();

    float interpolateValue(float time) const;

    float mapToParameterRange(float normalizedValue) const;

    float getPointValue(const EnvelopePoint &point) const;

    // Thread-safe envelope data with atomic pointer swapping
    mutable std::atomic<PointBuffer *> activePointBuffer;
    PointBuffer *editBuffer;

    // Timing data
    float currentTime = 0.0f;
    float rate = 1.0f;
    double currentPpqPosition = 0.0;
    bool useTransportSync = true;

    // Parameter settings
    EnvelopeParams::ParameterType parameterType;
    EnvelopeParams::ParameterSettings settings;
}; 