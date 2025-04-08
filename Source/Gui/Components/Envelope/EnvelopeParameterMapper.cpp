#include "EnvelopeParameterMapper.h"
#include "EnvelopePoint.h"
#include <cmath>

EnvelopeParameterMapper::EnvelopeParameterMapper(
        juce::Identifier paramId, TimingManager &tm
) : paramId(paramId), timingManager(tm) {
    auto initialBuffer = new PointBuffer();
    addDefaultPointsToBuffer(initialBuffer);

    editBuffer = createPointBufferCopy(initialBuffer).release();
    activePointBuffer.store(initialBuffer, std::memory_order_release);
}

EnvelopeParameterMapper::~EnvelopeParameterMapper() {
    delete activePointBuffer.load(std::memory_order_acquire);
    delete editBuffer;
}

std::unique_ptr<EnvelopeParameterMapper::PointBuffer>
EnvelopeParameterMapper::createPointBufferCopy(const PointBuffer *source) const {
    auto newBuffer = std::make_unique<PointBuffer>();
    if (source != nullptr) {
        for (const auto &point: source->points) {
            newBuffer->points.push_back(std::make_unique<EnvelopePoint>(
                    point->position.x, point->position.y));
            newBuffer->points.back()->curvature = point->curvature;
            newBuffer->points.back()->isEditable = point->isEditable;
            newBuffer->points.back()->selected = point->selected;
        }
    }
    return newBuffer;
}

void EnvelopeParameterMapper::addDefaultPointsToBuffer(PointBuffer *buffer) const {
    if (buffer == nullptr) return;

    buffer->points.clear();
    buffer->points.push_back(std::make_unique<EnvelopePoint>(0.0f, 0.5f));
    buffer->points.push_back(std::make_unique<EnvelopePoint>(0.5f, 0.5f));
    buffer->points.push_back(std::make_unique<EnvelopePoint>(1.0f, 0.5f));
}

float EnvelopeParameterMapper::getCurrentValue() const {
    double ppqPosition = timingManager.getPpqPosition();
    float normalizedPosition;

    if (useTransportSync && ppqPosition >= 0.0) {
        normalizedPosition = std::fmod(static_cast<float>(ppqPosition * rate), 1.0f);
    } else {
        normalizedPosition = std::fmod(currentTime * rate, 1.0f);
    }

    float normalizedValue = interpolateValue(normalizedPosition);

    if (settings.bipolar) {
        normalizedValue = normalizedValue * 2.0f - 1.0f;
    }

    float scaledModulation = normalizedValue * settings.amount;

    return scaledModulation;
}

void EnvelopeParameterMapper::setRate(float newRate) {
    rate = newRate;
}

void EnvelopeParameterMapper::setBipolar(bool isBipolar) {
    settings.bipolar = isBipolar;
}

void EnvelopeParameterMapper::setPoints(const std::vector<std::unique_ptr<EnvelopePoint>> &newPoints) {
    editBuffer->points.clear();
    for (const auto &point: newPoints) {
        editBuffer->points.push_back(std::make_unique<EnvelopePoint>(point->position.x, point->position.y));
        editBuffer->points.back()->curvature = point->curvature;
        editBuffer->points.back()->isEditable = point->isEditable;
        editBuffer->points.back()->selected = point->selected;
    }

    auto newActiveBuffer = createPointBufferCopy(editBuffer).release();
    auto oldBuffer = activePointBuffer.exchange(newActiveBuffer, std::memory_order_acq_rel);
    delete oldBuffer;
}

float EnvelopeParameterMapper::interpolateValue(float time) const {
    auto buffer = activePointBuffer.load(std::memory_order_acquire);
    const auto &points = buffer->points;

    if (points.empty()) return 0.5f;
    if (points.size() == 1) return points[0]->position.y;

    size_t i = 0;
    while (i < points.size() - 1 && points[i + 1]->position.x <= time) {
        ++i;
    }

    if (i >= points.size() - 1) {
        return points.back()->position.y;
    }

    const auto &p1 = points[i];
    const auto &p2 = points[i + 1];

    if (p2->curvature == 0.0f) {
        float t = (time - p1->position.x) / (p2->position.x - p1->position.x);
        return p1->position.y + t * (p2->position.y - p1->position.y);
    }

    float t = (time - p1->position.x) / (p2->position.x - p1->position.x);

    // Curvature: negative value = curve downward, positive value = curve upward
    const float curvature = p2->curvature;
    const float scaledCurvature = curvature * 0.7f;

    float linearValue = p1->position.y + t * (p2->position.y - p1->position.y);

    // Apply curvature: modify t to bend the curve
    // When t is 0 or 1, we stay at the endpoints
    // When t is in the middle, apply maximum curve effect
    float curveEffect = t * (1.0f - t); // Parabola that's 0 at t=0 and t=1, and maximum at t=0.5

    // Apply scaled curvature effect to the linear value
    // Negative curvature bends down, positive bends up
    return linearValue + (scaledCurvature * curveEffect);
}

float EnvelopeParameterMapper::mapToParameterRange(float normalizedValue) const {
    if (settings.exponential) {
        // For exponential parameters, use logarithmic mapping
        return settings.minValue * std::pow(settings.maxValue / settings.minValue, normalizedValue);
    } else {
        // For linear parameters, use simple linear mapping
        return settings.minValue + normalizedValue * (settings.maxValue - settings.minValue);
    }
}