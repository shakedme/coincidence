#include "EnvelopeParameterMapper.h"
#include "EnvelopePoint.h"
#include <cmath>

EnvelopeParameterMapper::EnvelopeParameterMapper() {
    // Create initial buffers
    auto initialBuffer = new PointBuffer();
    addDefaultPointsToBuffer(initialBuffer);

    // Create the edit buffer as a copy of the initial buffer
    editBuffer = createPointBufferCopy(initialBuffer).release();

    // Set the active buffer (the one read by audio thread)
    activePointBuffer.store(initialBuffer, std::memory_order_release);
}

EnvelopeParameterMapper::~EnvelopeParameterMapper() {
    // Clean up buffers
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
    // Add default points
    buffer->points.push_back(std::make_unique<EnvelopePoint>(0.0f, 0.5f));
    buffer->points.push_back(std::make_unique<EnvelopePoint>(0.5f, 0.5f));
    buffer->points.push_back(std::make_unique<EnvelopePoint>(1.0f, 0.5f));
}

float EnvelopeParameterMapper::getCurrentValue() const {
    // Use transport position if available
    if (useTransportSync && currentPpqPosition >= 0.0) {
        // Calculate where in the envelope we should be based on rate and PPQ position
        float normalizedPosition = std::fmod(static_cast<float>(currentPpqPosition * rate), 1.0f);

        float normalizedValue = interpolateValue(normalizedPosition);

        // For bipolar parameters, map 0-1 to -1 to 1 first
        if (settings.bipolar) {
            normalizedValue = normalizedValue * 2.0f - 1.0f;
        }

        return mapToParameterRange(normalizedValue);
    } else {
        // Fallback to time-based calculation if transport position is not available
        float normalizedTime = std::fmod(currentTime * rate, 1.0f);
        float normalizedValue = interpolateValue(normalizedTime);

        // For bipolar parameters, map 0-1 to -1 to 1 first
        if (settings.bipolar) {
            normalizedValue = normalizedValue * 2.0f - 1.0f;
        }

        return mapToParameterRange(normalizedValue);
    }
}

float EnvelopeParameterMapper::getValueAtPosition(float position) const {
    float normalizedValue = interpolateValue(position);

    // For bipolar parameters, map 0-1 to -1 to 1 first
    if (settings.bipolar) {
        normalizedValue = normalizedValue * 2.0f - 1.0f;
    }

    return mapToParameterRange(normalizedValue);
}

void EnvelopeParameterMapper::setTransportPosition(double ppqPosition) {
    currentPpqPosition = ppqPosition;
}

void EnvelopeParameterMapper::setRate(float newRate) {
    rate = newRate;
}

void EnvelopeParameterMapper::setParameterRange(float min, float max, bool isExponential) {
    settings.minValue = min;
    settings.maxValue = max;
    settings.exponential = isExponential;
}

void EnvelopeParameterMapper::setBipolar(bool isBipolar) {
    settings.bipolar = isBipolar;
}

void EnvelopeParameterMapper::setPoints(const std::vector<std::unique_ptr<EnvelopePoint>> &newPoints) {
    // Make changes to the edit buffer
    editBuffer->points.clear();
    for (const auto &point: newPoints) {
        editBuffer->points.push_back(std::make_unique<EnvelopePoint>(point->position.x, point->position.y));
        editBuffer->points.back()->curvature = point->curvature;
        editBuffer->points.back()->isEditable = point->isEditable;
        editBuffer->points.back()->selected = point->selected;
    }

    // Create a new active buffer
    auto newActiveBuffer = createPointBufferCopy(editBuffer).release();

    // Atomically swap the active buffer - this is the lock-free operation
    auto oldBuffer = activePointBuffer.exchange(newActiveBuffer, std::memory_order_acq_rel);

    // Delete the old buffer
    delete oldBuffer;
}

std::vector<std::unique_ptr<EnvelopePoint>> EnvelopeParameterMapper::getPointsCopy() const {
    // Read the active buffer safely
    auto buffer = activePointBuffer.load(std::memory_order_acquire);
    std::vector<std::unique_ptr<EnvelopePoint>> pointsCopy;

    for (const auto &point: buffer->points) {
        pointsCopy.push_back(std::make_unique<EnvelopePoint>(point->position.x, point->position.y));
        pointsCopy.back()->curvature = point->curvature;
        pointsCopy.back()->isEditable = point->isEditable;
        pointsCopy.back()->selected = point->selected;
    }

    return pointsCopy;
}

void EnvelopeParameterMapper::clearPoints() {
    // Make changes to the edit buffer
    addDefaultPointsToBuffer(editBuffer);

    // Create a new active buffer
    auto newActiveBuffer = createPointBufferCopy(editBuffer).release();

    // Atomically swap the active buffer
    auto oldBuffer = activePointBuffer.exchange(newActiveBuffer, std::memory_order_acq_rel);

    // Delete the old buffer
    delete oldBuffer;
}

float EnvelopeParameterMapper::interpolateValue(float time) const {
    // Get the current active buffer safely
    auto buffer = activePointBuffer.load(std::memory_order_acquire);
    const auto &points = buffer->points;

    if (points.empty()) return 0.5f;
    if (points.size() == 1) return points[0]->position.y;

    // Find the points to interpolate between
    size_t i = 0;
    while (i < points.size() - 1 && points[i + 1]->position.x <= time) {
        ++i;
    }

    if (i >= points.size() - 1) {
        return points.back()->position.y;
    }

    const auto &p1 = points[i];
    const auto &p2 = points[i + 1];

    // If there's no curvature, use linear interpolation
    if (p2->curvature == 0.0f) {
        float t = (time - p1->position.x) / (p2->position.x - p1->position.x);
        return p1->position.y + t * (p2->position.y - p1->position.y);
    }

    // For curved segments, use a simpler approach that directly respects the curvature
    float t = (time - p1->position.x) / (p2->position.x - p1->position.x);

    // Curvature: negative value = curve downward, positive value = curve upward
    const float curvature = p2->curvature;
    const float scaledCurvature = curvature * 0.7f; // Scale down for more reasonable effect

    // Calculate linear interpolation value
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