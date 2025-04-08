#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <atomic>
#include <memory>
#include "../../../Shared/TimingManager.h"

class EnvelopePoint;

struct ParameterSettings {
    bool bipolar = false;
    bool exponential = false;
    float amount = 1.0f;
    float minValue = 0.0f;
    float maxValue = 1.0f;
};

class EnvelopeParameterMapper {
public:
    EnvelopeParameterMapper(juce::Identifier paramId, TimingManager &timingManger);

    ~EnvelopeParameterMapper();

    float getCurrentValue() const;

    void setRate(float newRate);

    void setBipolar(bool isBipolar);

    juce::Identifier getParameterId() const { return paramId; }

    void setPoints(const std::vector<std::unique_ptr<EnvelopePoint>> &newPoints);

private:
    struct PointBuffer {
        std::vector<std::unique_ptr<EnvelopePoint>> points;
    };

    std::unique_ptr<PointBuffer> createPointBufferCopy(const PointBuffer *source) const;

    void addDefaultPointsToBuffer(PointBuffer *buffer) const;

    float interpolateValue(float time) const;

    float mapToParameterRange(float normalizedValue) const;

    juce::Identifier paramId;

    ParameterSettings settings;

    TimingManager &timingManager;

    float rate = 1.0f;
    bool useTransportSync = true;
    float currentTime = 0.0f;

    std::atomic<PointBuffer *> activePointBuffer;

    PointBuffer *editBuffer;
}; 