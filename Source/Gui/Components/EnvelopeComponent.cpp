#include "EnvelopeComponent.h"
#include <juce_graphics/juce_graphics.h>

//==============================================================================
EnvelopeComponent::EnvelopeComponent(EnvelopeParams::ParameterType type)
        : parameterMapper(type) {
    // Initialize with default points
    points.push_back(std::make_unique<EnvelopePoint>(0.0f, 0.5f, false));
    points.push_back(std::make_unique<EnvelopePoint>(1.0f, 0.5f, false));

    // Set up waveform rendering
    setupWaveformRendering();

    // Start the timer for waveform updates - use higher refresh rate for smoother animation
    startTimerHz(30); // 30 FPS for smoother visualization

    setWantsKeyboardFocus(true);
}

EnvelopeComponent::~EnvelopeComponent() {
    stopTimer();

    // Clean up the audio buffer queue
    if (audioBufferQueue != nullptr) {
        delete audioBufferQueue;
        audioBufferQueue = nullptr;
    }
}

void EnvelopeComponent::paint(juce::Graphics &g) {
    // Draw background
    g.fillAll(juce::Colour(0xff222222));

    // Draw waveform first (behind everything else)
    drawWaveform(g);

    // Draw grid
    drawGrid(g);

    // Draw envelope line
    drawEnvelopeLine(g);
    
    // Draw current position marker
    drawPositionMarker(g);

    // Draw points
    drawPoints(g);

    // Draw selection area if active
    if (isCreatingSelectionArea) {
        drawSelectionArea(g);
    }
}

void EnvelopeComponent::resized() {
    // Update waveform data buffer when component is resized
    if (waveformData.size() != static_cast<size_t>(getWidth())) {
        waveformData.resize(getWidth(), 0.0f);
        waveformPeaks.resize(getWidth());
        waveformCache = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
        waveformNeedsRedraw.store(true);
    }
}

void EnvelopeComponent::mouseDown(const juce::MouseEvent &e) {
    // Check if clicking on an existing point
    for (auto &point: points) {
        juce::Point<float> pos = getPointScreenPosition(*point);
        if (pos.getDistanceFrom(e.position.toFloat()) < pointRadius) {
            // If clicking on an already selected point, don't clear other selections
            // unless shift is not held
            if (!point->selected && !e.mods.isShiftDown()) {
                // Clear previous selection if clicking on an unselected point
                for (auto &p: points)
                    p->selected = false;
            }

            point->selected = true;
            lastDragPosition = e.position;
            if (point->isEditable) {
                if (getSelectedPointsCount() == 1) {
                    pointDragging = point.get();
                    isDraggingSelectedPoints = false;
                } else {
                    pointDragging = nullptr;
                    isDraggingSelectedPoints = true;
                }
            }
            repaint();
            return;
        }
    }

    // Check if Alt is held and clicking on a line segment to adjust curvature
    if (e.mods.isAltDown()) {
        // Find the line segment closest to the click point
        juce::Point<float> clickPos = e.position.toFloat();
        int segmentIndex = findClosestSegmentIndex(clickPos);
        if (segmentIndex >= 0) {
            curveEditingSegment = segmentIndex;
            // Start with current curvature value
            initialCurvature = points[segmentIndex + 1]->curvature;
            curveEditStartPos = e.position;
            repaint();
            return;
        }
    }

    // If not clicking on a point or line segment, start creating a selection area
    if (!e.mods.isAltDown()) {
        // Clear current selection if not adding to it
        if (!e.mods.isShiftDown()) {
            for (auto &p: points)
                p->selected = false;
        }

        isCreatingSelectionArea = true;
        selectionStart = e.position;
        selectionArea.setPosition(selectionStart);
        selectionArea.setSize(0, 0);
        repaint();
    }
}

void EnvelopeComponent::mouseDrag(const juce::MouseEvent &e) {
    if (pointDragging != nullptr) {
        // Update the point position, keeping it within bounds
        float normX = (float) e.x / getWidth();
        float normY = 1.0f - (float) e.y / getHeight(); // Invert Y to match the display
        normX = juce::jlimit(0.0f, 1.0f, normX);
        normY = juce::jlimit(0.0f, 1.0f, normY);

        pointDragging->position.setXY(normX, normY);

        // Keep points sorted by x position
        std::sort(points.begin(), points.end(),
                  [](const std::unique_ptr<EnvelopePoint> &a, const std::unique_ptr<EnvelopePoint> &b) {
                      return a->position.x < b->position.x;
                  });

        // Update parameter mapper
        parameterMapper.setPoints(points);
        notifyPointsChanged();
        repaint();
    } else if (isDraggingSelectedPoints) {
        // Move all selected points together
        juce::Point<float> delta = e.position - lastDragPosition;

        // Convert delta to normalized coordinates
        float normDeltaX = delta.x / (float) getWidth();
        float normDeltaY = -delta.y / (float) getHeight(); // Invert Y for display coordinates

        // Move each selected point
        for (auto &point: points) {
            if (point->selected) {
                // Apply movement while keeping points within bounds
                float newX = juce::jlimit(0.0f, 1.0f, point->position.x + normDeltaX);
                float newY = juce::jlimit(0.0f, 1.0f, point->position.y + normDeltaY);
                point->position.setXY(newX, newY);
            }
        }

        // Keep points sorted by x position
        std::sort(points.begin(), points.end(),
                  [](const std::unique_ptr<EnvelopePoint> &a, const std::unique_ptr<EnvelopePoint> &b) {
                      return a->position.x < b->position.x;
                  });

        // Update parameter mapper
        parameterMapper.setPoints(points);
        notifyPointsChanged();
        lastDragPosition = e.position;
        repaint();
    } else if (curveEditingSegment >= 0 && curveEditingSegment < points.size() - 1) {
        // Calculate curve adjustment based on vertical drag distance
        // Make the direction intuitive: dragging down creates a downward curve (negative value)
        float verticalDelta = (e.position.getY() - curveEditStartPos.getY()) / 100.0f;

        // Adjust the curvature of the line segment (next point stores the curve)
        points[curveEditingSegment + 1]->curvature = juce::jlimit(-1.0f, 1.0f, initialCurvature + verticalDelta);

        // Update parameter mapper
        parameterMapper.setPoints(points);
        notifyPointsChanged();
        repaint();
    } else if (isCreatingSelectionArea) {
        // Update the selection rectangle
        float left = std::min(selectionStart.x, e.position.x);
        float top = std::min(selectionStart.y, e.position.y);
        float right = std::max(selectionStart.x, e.position.x);
        float bottom = std::max(selectionStart.y, e.position.y);

        selectionArea.setBounds(left, top, right - left, bottom - top);

        // Select points within the area as we drag
        selectPointsInArea();

        repaint();
    }
}

void EnvelopeComponent::mouseUp(const juce::MouseEvent &) {
    pointDragging = nullptr;
    curveEditingSegment = -1;
    isDraggingSelectedPoints = false;

    // Finalize the selection area
    if (isCreatingSelectionArea) {
        isCreatingSelectionArea = false;

        // If the selection area is very small, it might have been an accidental click
        // In that case, clear the selection
        if (selectionArea.getWidth() < 5 && selectionArea.getHeight() < 5) {
            for (auto &p: points)
                p->selected = false;
        }

        repaint();
    }

}

void EnvelopeComponent::mouseDoubleClick(const juce::MouseEvent &e) {
    // First check if double-clicking on an existing point to delete it
    for (auto it = points.begin(); it != points.end(); ++it) {
        juce::Point<float> pos = getPointScreenPosition(*(*it));
        if (pos.getDistanceFrom(e.position.toFloat()) < pointRadius) {
            // Don't allow deleting the first or last point
            if (it != points.begin() && it != std::prev(points.end())) {
                points.erase(it);
                // Update parameter mapper
                parameterMapper.setPoints(points);
                notifyPointsChanged();
                repaint();
            }
            return;
        }
    }

    // Check if clicking on a curve segment to reset curvature
    int segmentIndex = findClosestSegmentIndex(e.position.toFloat());
    if (segmentIndex >= 0) {
        points[segmentIndex + 1]->curvature = 0.0f;
        // Update parameter mapper
        parameterMapper.setPoints(points);
        notifyPointsChanged();
        repaint();
        return;
    }

    // If not clicking on a point, create a new one at the double-click position
    float normX = (float) e.x / getWidth();
    float normY = 1.0f - (float) e.y / getHeight(); // Invert Y to match the display
    normX = juce::jlimit(0.0f, 1.0f, normX);
    normY = juce::jlimit(0.0f, 1.0f, normY);

    auto newPoint = std::make_unique<EnvelopePoint>(normX, normY);
    newPoint->selected = true;

    for (auto &p: points)
        p->selected = false;

    // Insert at the correct position based on x-coordinate
    auto it = std::upper_bound(points.begin(), points.end(), newPoint,
                               [](const std::unique_ptr<EnvelopePoint> &a, const std::unique_ptr<EnvelopePoint> &b) {
                                   return a->position.x < b->position.x;
                               });

    points.insert(it, std::move(newPoint));

    // Update parameter mapper
    parameterMapper.setPoints(points);
    notifyPointsChanged();

    // Reset dragging state immediately
    pointDragging = nullptr;
    isDraggingSelectedPoints = false;
    lastDragPosition = e.position;

    repaint();
}

void EnvelopeComponent::setWaveformScaleFactor(float scale) {
    waveformScaleFactor = scale;
    repaint();
}

void EnvelopeComponent::setSampleRate(float newSampleRate) {
    sampleRate = newSampleRate;
    waveformNeedsRedraw.store(true);
}

void EnvelopeComponent::setTimeRange(float seconds) {
    timeRangeInSeconds = seconds;
    waveformNeedsRedraw.store(true);
}

void EnvelopeComponent::drawGrid(juce::Graphics &g) {
    const float width = getWidth();
    const float height = getHeight();

    g.setColour(juce::Colour(0xff444444));

    // Draw vertical lines
    for (int i = 0; i <= horizontalDivisions; ++i) {
        const float x = i * (width / horizontalDivisions);
        g.drawLine(x, 0, x, height, 1.0f);
    }

    // Draw horizontal lines
    for (int i = 0; i <= verticalDivisions; ++i) {
        const float y = i * (height / verticalDivisions);
        g.drawLine(0, y, width, y, 1.0f);
    }

    // Draw center line (value 50%)
    g.setColour(juce::Colour(0xff666666));
    g.drawLine(0, height / 2, width, height / 2, 1.5f);
}

void EnvelopeComponent::drawEnvelopeLine(juce::Graphics &g) {
    if (points.size() < 2) return;

    g.setColour(juce::Colour(0xff52bfd9));

    // Create a path to draw the envelope curve
    juce::Path path;

    juce::Point<float> startPos = getPointScreenPosition(*points[0]);
    path.startNewSubPath(startPos);

    for (size_t i = 1; i < points.size(); ++i) {
        juce::Point<float> endPos = getPointScreenPosition(*points[i]);

        if (points[i]->curvature != 0.0f) {
            // Calculate control points for a quadratic bezier curve
            const float curvature = points[i]->curvature;
            
            // For visual display, use a large multiplier, but invert the sign to match parameter behavior
            // Negative curvature = bend down, positive curvature = bend up
            const float curveAmount = -100.0f * curvature; // Invert direction for visual consistency
            
            // Calculate control point position
            juce::Point<float> midPoint = startPos + (endPos - startPos) * 0.5f;
            juce::Point<float> perpendicular(-((endPos.y - startPos.y)), (endPos.x - startPos.x));

            // Manually normalize the perpendicular vector
            float length = std::sqrt(perpendicular.x * perpendicular.x + perpendicular.y * perpendicular.y);
            if (length > 0.0f) {
                perpendicular.x = perpendicular.x / length * curveAmount;
                perpendicular.y = perpendicular.y / length * curveAmount;
            }

            juce::Point<float> controlPoint = midPoint + perpendicular;

            // Draw the curved segment
            path.quadraticTo(controlPoint, endPos);
        } else {
            // Draw straight line
            path.lineTo(endPos);
        }

        startPos = endPos;
    }

    g.strokePath(path, juce::PathStrokeType(2.0f));
}

void EnvelopeComponent::drawPoints(juce::Graphics &g) {
    for (const auto &point: points) {
        juce::Point<float> pos = getPointScreenPosition(*point);

        // Draw point
        if (point->selected) {
            g.setColour(juce::Colours::white);
            g.fillEllipse(pos.x - pointRadius, pos.y - pointRadius,
                          pointRadius * 2, pointRadius * 2);

            g.setColour(juce::Colour(0xff52bfd9));
            g.drawEllipse(pos.x - pointRadius, pos.y - pointRadius,
                          pointRadius * 2, pointRadius * 2, 2.0f);
        } else {
            g.setColour(juce::Colour(0xff52bfd9));
            g.fillEllipse(pos.x - pointRadius, pos.y - pointRadius,
                          pointRadius * 2, pointRadius * 2);
        }

        // If point has curvature, indicate it with a small mark
        if (point->curvature != 0.0f) {
            g.setColour(juce::Colours::yellow);
            g.fillEllipse(pos.x - 2.0f, pos.y - 2.0f, 4.0f, 4.0f);
        }
    }
}

juce::Point<float> EnvelopeComponent::getPointScreenPosition(const EnvelopePoint &point) const {
    // Convert normalized position to screen coordinates
    // Note that Y is inverted in GUI coordinates (0 is top)
    return juce::Point<float>(
            point.position.x * getWidth(),
            (1.0f - point.position.y) * getHeight() // Invert Y
    );
}

int EnvelopeComponent::findClosestSegmentIndex(const juce::Point<float> &clickPos) const {
    int segmentIndex = -1;
    float minDistance = std::numeric_limits<float>::max();

    for (int i = 0; i < points.size() - 1; ++i) {
        juce::Point<float> p1 = getPointScreenPosition(*points[i]);
        juce::Point<float> p2 = getPointScreenPosition(*points[i + 1]);

        float dist;
        // If the segment has curvature, check distance to the curve
        if (points[i + 1]->curvature != 0.0f) {
            dist = distanceToCurve(clickPos, p1, p2, points[i + 1]->curvature);
        } else {
            // For straight lines, use the original calculation
            dist = distanceToLineSegment(clickPos, p1, p2);
        }

        if (dist < minDistance && dist < 10.0f) // 10-pixel threshold for line selection
        {
            minDistance = dist;
            segmentIndex = i;
        }
    }

    return segmentIndex;
}

float EnvelopeComponent::distanceToLineSegment(const juce::Point<float> &p, const juce::Point<float> &v,
                                               const juce::Point<float> &w) const {
    // Return minimum distance between line segment vw and point p
    const float l2 = v.getDistanceSquaredFrom(w);  // length squared of segment vw

    // If segment is a point, return distance to the point
    if (l2 == 0.0f)
        return p.getDistanceFrom(v);

    // Consider the line extending the segment, parameterized as v + t (w - v)
    // We find projection of point p onto the line.
    // Projection falls on the segment if t is between 0 and 1
    const float t = juce::jlimit(0.0f, 1.0f, ((p.x - v.x) * (w.x - v.x) + (p.y - v.y) * (w.y - v.y)) / l2);

    // Projection point on the line segment
    const juce::Point<float> projection = {v.x + t * (w.x - v.x), v.y + t * (w.y - v.y)};

    return p.getDistanceFrom(projection);
}

float EnvelopeComponent::distanceToCurve(const juce::Point<float> &point, const juce::Point<float> &start,
                                         const juce::Point<float> &end, float curvature) const {
    // Sample several points along the curve and find the closest one
    const int numSamples = 20;
    float minDistance = std::numeric_limits<float>::max();

    // Calculate the control point for the quadratic bezier
    // Use the same inversion as in drawEnvelopeLine for consistency
    const float curveAmount = -100.0f * curvature; // Invert direction to match visual display
    juce::Point<float> midPoint = start + (end - start) * 0.5f;
    juce::Point<float> perpendicular(-((end.y - start.y)), (end.x - start.x));

    // Normalize the perpendicular vector
    float length = std::sqrt(perpendicular.x * perpendicular.x + perpendicular.y * perpendicular.y);
    if (length > 0.0f) {
        perpendicular.x = perpendicular.x / length * curveAmount;
        perpendicular.y = perpendicular.y / length * curveAmount;
    }

    juce::Point<float> controlPoint = midPoint + perpendicular;

    // Sample points along the curve and find the closest one
    for (int i = 0; i <= numSamples; ++i) {
        float t = (float) i / numSamples;

        // Quadratic bezier formula: B(t) = (1-t)²P₀ + 2(1-t)tP₁ + t²P₂
        float oneMinusT = 1.0f - t;
        float oneMinusTSquared = oneMinusT * oneMinusT;
        float tSquared = t * t;

        juce::Point<float> samplePoint(
                oneMinusTSquared * start.x + 2.0f * oneMinusT * t * controlPoint.x + tSquared * end.x,
                oneMinusTSquared * start.y + 2.0f * oneMinusT * t * controlPoint.y + tSquared * end.y
        );

        float distance = point.getDistanceFrom(samplePoint);
        minDistance = std::min(minDistance, distance);
    }

    return minDistance;
}

void EnvelopeComponent::setupWaveformRendering() {
    // Create the buffer queue
    audioBufferQueue = new AudioBufferQueue();

    // Initialize waveform data buffer based on component width
    waveformData.resize(getWidth(), 0.0f);

    // Create the initial cache image
    waveformCache = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
}

void EnvelopeComponent::pushAudioBuffer(const float *audioData, int numSamples) {
    // This method is called from the audio thread
    // Add data to the ring buffer in a lock-free manner
    if (audioBufferQueue != nullptr) {
        audioBufferQueue->push(audioData, numSamples);
        waveformNeedsRedraw.store(true);
    }
}

void EnvelopeComponent::timerCallback() {
    // Update transport position if timing manager is available
    if (timingManager != nullptr) {
        double ppqPosition = timingManager->getPpqPosition();
        parameterMapper.setTransportPosition(ppqPosition);
        
        // Always repaint to show current envelope position in sync with transport
        repaint();
    }

    // Check if we need to update the waveform
    if (waveformNeedsRedraw.load()) {
        // Use mutex to protect the cache update, but not the data acquisition
        updateWaveformCache();
        waveformNeedsRedraw.store(false);
        repaint();
    }
}

void EnvelopeComponent::updateWaveformCache() {
    // Check if component has been resized
    if (waveformData.size() != static_cast<size_t>(getWidth()) && getWidth() > 0 && !waveformData.empty()) {
        waveformData.resize(getWidth(), 0.0f);
        waveformCache = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
    }

    // Fetch the audio data for the visible time range
    if (audioBufferQueue != nullptr) {
        // Calculate how many samples we need based on the time range
        size_t samplesForTimeRange = static_cast<size_t>(timeRangeInSeconds * sampleRate);

        // Resize waveformData if necessary to match time range
        if (waveformData.size() != samplesForTimeRange) {
            waveformData.resize(samplesForTimeRange, 0.0f);
        }

        // Get samples for the visible range - we want the most recent samples, so offset is 0
        audioBufferQueue->getVisibleSamples(waveformData.data(), waveformData.size(), 0);
    }

    // Prepare the peak data structure
    waveformPeaks.resize(getWidth());

    // Process the raw sample data into min-max peaks for each pixel column
    const float samplesPerPixel = static_cast<float>(waveformData.size()) / getWidth();

    // For each horizontal pixel in our display
    for (int x = 0; x < getWidth(); ++x) {
        float min = 0.0f;
        float max = 0.0f;

        // For left-to-right flow, we need to reverse the x index used to look up samples
        // This makes newest samples appear on the right and older samples flow left
        int reverseX = getWidth() - 1 - x;

        // Calculate the range of samples that correspond to this pixel
        int startSample = static_cast<int>(reverseX * samplesPerPixel);
        int endSample = static_cast<int>((reverseX + 1) * samplesPerPixel);

        // Ensure we don't exceed the buffer bounds
        startSample = juce::jlimit(0, static_cast<int>(waveformData.size()) - 1, startSample);
        endSample = juce::jlimit(0, static_cast<int>(waveformData.size()), endSample);

        // If we have at least one sample, initialize min and max
        if (startSample < endSample && startSample < waveformData.size()) {
            min = max = waveformData[startSample];

            // Find min and max values in this sample range
            for (int i = startSample + 1; i < endSample && i < waveformData.size(); ++i) {
                float sample = waveformData[i];
                min = std::min(min, sample);
                max = std::max(max, sample);
            }
        }

        // Store the min-max values for this pixel
        waveformPeaks[x].min = min;
        waveformPeaks[x].max = max;
    }

    // Lock only the cache update portion, not the data fetching
    std::lock_guard<std::mutex> lock(waveformMutex);

    // Clear the cache
    juce::Graphics g(waveformCache);
    g.setColour(juce::Colours::transparentBlack);
    g.fillAll();

    // Draw the waveform with more transparency
    g.setColour(waveformColour.withAlpha(0.3f));

    const float midY = getHeight() / 2.0f;
    const float scaleFactor = midY * waveformScaleFactor;

    // Draw waveform using high-resolution peaks
    for (int x = 0; x < getWidth(); ++x) {
        // Get min and max values for this pixel
        float minVal = waveformPeaks[x].min;
        float maxVal = waveformPeaks[x].max;

        // Convert to screen coordinates
        float minY = midY - (minVal * scaleFactor);
        float maxY = midY - (maxVal * scaleFactor);

        // Draw a vertical line from min to max
        if (std::abs(maxVal - minVal) > 0.001f) // Only draw if there's a meaningful difference
        {
            g.drawLine(static_cast<float>(x), minY, static_cast<float>(x), maxY, 1.0f);
        } else {
            // For very small differences, just draw a point
            g.drawLine(static_cast<float>(x), minY - 1.0f, static_cast<float>(x), maxY + 1.0f, 1.0f);
        }
    }

    // For very detailed waveforms, we can also draw a center line connecting average values
    juce::Path centerPath;
    bool centerPathStarted = false;

    for (int x = 0; x < getWidth(); ++x) {
        // Calculate the average value at this point
        float avgY = midY - ((waveformPeaks[x].min + waveformPeaks[x].max) * 0.5f * scaleFactor);

        if (!centerPathStarted) {
            centerPath.startNewSubPath(static_cast<float>(x), avgY);
            centerPathStarted = true;
        } else {
            centerPath.lineTo(static_cast<float>(x), avgY);
        }
    }

    // Draw the center line with a slightly more visible color
    g.setColour(waveformColour.withAlpha(0.5f));
    g.strokePath(centerPath, juce::PathStrokeType(1.0f));
}

void EnvelopeComponent::drawWaveform(juce::Graphics &g) {
    std::lock_guard<std::mutex> lock(waveformMutex);

    // Just draw the pre-rendered cache
    g.drawImageAt(waveformCache, 0, 0);
}


void EnvelopeComponent::drawSelectionArea(juce::Graphics &g) {
    // Semi-transparent fill
    g.setColour(juce::Colour(0x3052bfd9)); // Light blue with 30% opacity
    g.fillRect(selectionArea);

    // Border
    g.setColour(juce::Colour(0xff52bfd9)); // Solid light blue
    g.drawRect(selectionArea, 1.0f);
}

void EnvelopeComponent::selectPointsInArea() {
    // Check each point to see if it's within the selection area
    for (auto &point: points) {
        juce::Point<float> pos = getPointScreenPosition(*point);

        // Check if the point is inside the selection rectangle
        if (selectionArea.contains(pos)) {
            point->selected = true;
        }
    }
}

int EnvelopeComponent::getSelectedPointsCount() const {
    int count = 0;
    for (const auto &point: points) {
        if (point->selected) {
            count++;
        }
    }
    return count;
}

bool EnvelopeComponent::keyPressed(const juce::KeyPress &key) {
    // Handle backspace key to delete selected points
    if (key == juce::KeyPress::backspaceKey) {
        // Don't allow deleting first or last point
        bool hasFirstOrLastSelected = points.front()->selected || points.back()->selected;
        if (!hasFirstOrLastSelected && getSelectedPointsCount() > 0) {
            // Remove all selected points except first and last
            points.erase(
                    std::remove_if(points.begin() + 1, points.end() - 1,
                                   [](const std::unique_ptr<EnvelopePoint> &point) {
                                       return point->selected;
                                   }
                    ),
                    points.end() - 1
            );
            repaint();
            return true;
        }
    }
    return false;
}

// Parameter mapping methods
void EnvelopeComponent::setParameterRange(float min, float max, bool exponential) {
    parameterMapper.setParameterRange(min, max, exponential);
}

float EnvelopeComponent::getCurrentValue() const {
    return parameterMapper.getCurrentValue();
}

void EnvelopeComponent::updateTime(float deltaTime) {
    parameterMapper.updateTime(deltaTime);
}

void EnvelopeComponent::setRate(float newRate) {
    parameterMapper.setRate(newRate);
    // Notify listeners of rate change if callback is set
    if (onRateChanged) {
        onRateChanged(newRate);
    }
}

void EnvelopeComponent::notifyPointsChanged() {
    if (onPointsChanged) {
        onPointsChanged();
    }
}

void EnvelopeComponent::setParameterType(EnvelopeParams::ParameterType type) {
    parameterMapper.setParameterType(type);
    repaint();
}

EnvelopeParams::ParameterType EnvelopeComponent::getParameterType() const {
    return parameterMapper.getParameterType();
}

// Add this new method to draw the position marker
void EnvelopeComponent::drawPositionMarker(juce::Graphics &g) {
    if (timingManager == nullptr)
        return;
    
    // Calculate current position in the envelope cycle
    double ppqPosition = timingManager->getPpqPosition();
    float cycle = std::fmod(static_cast<float>(ppqPosition * parameterMapper.getRate()), 1.0f);
    
    // Convert to screen x-coordinate
    float x = cycle * getWidth();
    
    // Draw vertical line at current position
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawLine(x, 0, x, static_cast<float>(getHeight()), 1.0f);
    
    // Add a small indicator at the top
    g.setColour(juce::Colours::white);
    g.fillRoundedRectangle(x - 2, 0, 4, 8, 2);
} 