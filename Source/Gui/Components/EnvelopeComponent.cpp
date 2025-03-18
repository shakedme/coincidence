#include "EnvelopeComponent.h"
#include <juce_graphics/juce_graphics.h>

//==============================================================================
EnvelopeComponent::EnvelopeComponent(TimingManager &tm, EnvelopeParams::ParameterType type)
        : timingManager(tm), parameterMapper(type) {
    // Initialize with default points
    points.push_back(std::make_unique<EnvelopePoint>(0.0f, 0.5f, false));
    points.push_back(std::make_unique<EnvelopePoint>(1.0f, 0.5f, false));

    // Add the waveform component as a child component
    addAndMakeVisible(waveformComponent);

    // Configure the waveform component to be transparent so we can draw over it
    waveformComponent.setBackgroundColour(juce::Colours::transparentBlack);
    waveformComponent.setWaveformAlpha(0.3f);
    waveformComponent.setWaveformColour(juce::Colour(0xff52bfd9));
    waveformComponent.setWaveformScaleFactor(1.0f);

    // Setup the rate UI
    setupRateUI();
    
    // Setup the presets UI
    setupPresetsUI();
    
    // Setup the snap-to-grid UI
    setupSnapToGridUI();

    // Start the timer for envelope updates
    startTimerHz(30); // 30 FPS for smoother visualization

    juce::Timer::callAfterDelay(200, [this]() {
        updateTimeRangeFromRate();
    });

    setWantsKeyboardFocus(true);
}

EnvelopeComponent::~EnvelopeComponent() {
    stopTimer();
}

void EnvelopeComponent::paint(juce::Graphics &g) {
    // Draw background
    g.fillAll(juce::Colour(0xff222222));

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
    // Calculate space for the rate controls
    const int controlHeight = 25;
    const int labelWidth = 40;
    const int comboWidth = 60;
    const int padding = 5;
    const int topControlArea = controlHeight + (2 * padding);
    
    // Position rate controls at the top left
    rateLabel->setBounds(padding, padding, labelWidth, controlHeight);
    rateComboBox->setBounds(labelWidth + (2 * padding), padding, comboWidth, controlHeight);
    
    // Position preset shapes controls at the top right
    const int presetLabelWidth = 60;
    const int presetComboWidth = 90;
    presetShapesLabel->setBounds(getWidth() - presetLabelWidth - presetComboWidth - (2 * padding), 
                                padding, presetLabelWidth, controlHeight);
    presetShapesComboBox->setBounds(getWidth() - presetComboWidth - padding, 
                                  padding, presetComboWidth, controlHeight);
    
    // Position snap-to-grid button at the center top
    const int snapButtonWidth = 100;
    snapToGridButton->setBounds((getWidth() - snapButtonWidth) / 2, padding, snapButtonWidth, controlHeight);

    // Position the waveform component below the controls
    auto waveformBounds = getLocalBounds();
    waveformBounds.removeFromTop(topControlArea);
    waveformComponent.setBounds(waveformBounds);
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
        
        // For first and last points, maintain their fixed x position (only allow vertical movement)
        if (pointDragging == points.front().get() || pointDragging == points.back().get()) {
            normX = pointDragging->position.x; // Keep original X position
        }
        
        // Apply snap to grid if enabled
        if (snapToGridFlag) {
            juce::Point<float> normalizedPos(normX, normY);
            juce::Point<float> snappedPos = snapToGrid(normalizedPos);
            
            // For first and last points, only apply vertical snapping
            if (pointDragging == points.front().get() || pointDragging == points.back().get()) {
                normY = snappedPos.y;
            } else {
                normX = snappedPos.x;
                normY = snappedPos.y;
            }
        }

        pointDragging->position.setXY(normX, normY);

        // Keep points sorted by x position
        std::sort(points.begin(), points.end(),
                  [](const std::unique_ptr<EnvelopePoint> &a, const std::unique_ptr<EnvelopePoint> &b) {
                      return a->position.x < b->position.x;
                  });
                  
        // Set the current preset shape to Custom since user is editing points
        currentPresetShape = PresetShape::Custom;
        presetShapesComboBox->setSelectedId(static_cast<int>(PresetShape::Custom) + 1, juce::dontSendNotification);

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
                // For first and last points, only apply vertical movement
                float newX, newY;
                
                if (point.get() == points.front().get() || point.get() == points.back().get()) {
                    newX = point->position.x; // Keep original X position
                    newY = juce::jlimit(0.0f, 1.0f, point->position.y + normDeltaY);
                } else {
                    // Apply movement while keeping points within bounds
                    newX = juce::jlimit(0.0f, 1.0f, point->position.x + normDeltaX);
                    newY = juce::jlimit(0.0f, 1.0f, point->position.y + normDeltaY);
                }
                
                // Apply snap to grid if enabled
                if (snapToGridFlag) {
                    juce::Point<float> normalizedPos(newX, newY);
                    juce::Point<float> snappedPos = snapToGrid(normalizedPos);
                    
                    // For first and last points, only apply vertical snapping
                    if (point.get() == points.front().get() || point.get() == points.back().get()) {
                        newY = snappedPos.y;
                    } else {
                        newX = snappedPos.x;
                        newY = snappedPos.y;
                    }
                }
                
                point->position.setXY(newX, newY);
            }
        }

        // Keep points sorted by x position
        std::sort(points.begin(), points.end(),
                  [](const std::unique_ptr<EnvelopePoint> &a, const std::unique_ptr<EnvelopePoint> &b) {
                      return a->position.x < b->position.x;
                  });
                  
        // Set the current preset shape to Custom since user is editing points
        currentPresetShape = PresetShape::Custom;
        presetShapesComboBox->setSelectedId(static_cast<int>(PresetShape::Custom) + 1, juce::dontSendNotification);

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
                
                // Set the current preset shape to Custom since points changed
                currentPresetShape = PresetShape::Custom;
                presetShapesComboBox->setSelectedId(static_cast<int>(PresetShape::Custom) + 1, juce::dontSendNotification);
                
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
        
        // Set the current preset shape to Custom since curves changed
        currentPresetShape = PresetShape::Custom;
        presetShapesComboBox->setSelectedId(static_cast<int>(PresetShape::Custom) + 1, juce::dontSendNotification);
        
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
    
    // Apply snap to grid if enabled
    if (snapToGridFlag) {
        juce::Point<float> normalizedPos(normX, normY);
        juce::Point<float> snappedPos = snapToGrid(normalizedPos);
        normX = snappedPos.x;
        normY = snappedPos.y;
    }

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
    
    // Set the current preset shape to Custom since user added a point
    currentPresetShape = PresetShape::Custom;
    presetShapesComboBox->setSelectedId(static_cast<int>(PresetShape::Custom) + 1, juce::dontSendNotification);

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
    waveformComponent.setWaveformScaleFactor(scale);
}

void EnvelopeComponent::setSampleRate(float newSampleRate) {
    // Pass the sample rate to the waveform component
    waveformComponent.setSampleRate(newSampleRate);
}

void EnvelopeComponent::setTimeRange(float seconds) {
    // Pass the time range to the waveform component
    waveformComponent.setTimeRange(seconds);
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
    
    // If snap to grid is enabled, make grid slightly brighter
    if (snapToGridFlag) {
        g.setColour(juce::Colour(0xff888888));
        
        // Draw vertical snap lines
        for (int i = 0; i <= horizontalDivisions; ++i) {
            const float x = i * (width / horizontalDivisions);
            g.drawLine(x, 0, x, height, 0.5f);
        }
        
        // Draw horizontal snap lines
        for (int i = 0; i <= verticalDivisions; ++i) {
            const float y = i * (height / verticalDivisions);
            g.drawLine(0, y, width, y, 0.5f);
        }
    }
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
            parameterMapper.setPoints(points);
            notifyPointsChanged();
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
        onPointsChanged(getPoints());
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
    // Calculate current position in the envelope cycle
    double ppqPosition = timingManager.getPpqPosition();
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

void EnvelopeComponent::pushAudioBuffer(const float *audioData, int numSamples) {
    // This method is called from the audio thread
    // Forward the audio data to the waveform component
    waveformComponent.pushAudioBuffer(audioData, numSamples);
}

void EnvelopeComponent::timerCallback() {
    // Update transport position if timing manager is available
    double ppqPosition = timingManager.getPpqPosition();
    parameterMapper.setTransportPosition(ppqPosition);
    // Always repaint to show current envelope position in sync with transport
    repaint();
}

// Add setupRateUI method
void EnvelopeComponent::setupRateUI() {
    // Create rate label
    rateLabel = std::make_unique<juce::Label>("rateLabel", "Rate:");
    rateLabel->setFont(juce::Font(14.0f));
    rateLabel->setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(rateLabel.get());

    // Create rate combo box
    rateComboBox = std::make_unique<juce::ComboBox>("rateComboBox");
    rateComboBox->addItem("2/1", static_cast<int>(Rate::TwoWhole) + 1);
    rateComboBox->addItem("1/1", static_cast<int>(Rate::Whole) + 1);
    rateComboBox->addItem("1/2", static_cast<int>(Rate::Half) + 1);
    rateComboBox->addItem("1/4", static_cast<int>(Rate::Quarter) + 1);
    rateComboBox->addItem("1/8", static_cast<int>(Rate::Eighth) + 1);
    rateComboBox->addItem("1/16", static_cast<int>(Rate::Sixteenth) + 1);
    rateComboBox->addItem("1/32", static_cast<int>(Rate::ThirtySecond) + 1);
    rateComboBox->setSelectedId(static_cast<int>(Rate::Whole) + 1);
    rateComboBox->onChange = [this] {
        updateRateFromComboBox();
    };
    addAndMakeVisible(rateComboBox.get());
}

void EnvelopeComponent::updateRateFromComboBox() {
    // Update the current rate enum
    currentRateEnum = static_cast<Rate>(rateComboBox->getSelectedId() - 1);

    // Calculate and set the appropriate rate based on selection
    float newRate = 1.0f;
    switch (currentRateEnum) {
        case Rate::TwoWhole:
            newRate = 0.125f;
            break;
        case Rate::Whole:
            newRate = 0.25f;
            break;
        case Rate::Half:
            newRate = 0.5f;
            break;
        case Rate::Quarter:
            newRate = 1.0f;
            break;
        case Rate::Eighth:
            newRate = 2.0f;
            break;
        case Rate::Sixteenth:
            newRate = 4.0f;
            break;
        case Rate::ThirtySecond:
            newRate = 8.0f;
            break;
    }

    // Set the rate on the parameter mapper
    setRate(newRate);
    updateTimeRangeFromRate();
}

void EnvelopeComponent::updateTimeRangeFromRate() {
    const double bpm = timingManager.getBpm();
    const double beatsPerSecond = bpm / 60.0;

    // Calculate time range based on selected rate - this should match exactly one envelope cycle
    float timeRangeInSeconds = 1.0f;

    switch (currentRateEnum) {
        case Rate::TwoWhole:
            timeRangeInSeconds = static_cast<float>(8.0 / beatsPerSecond);
            break;
        case Rate::Whole:
            // One whole note = 4 quarter notes
            timeRangeInSeconds = static_cast<float>(4.0 / beatsPerSecond);
            break;
        case Rate::Half:
            // One half note = 2 quarter notes
            timeRangeInSeconds = static_cast<float>(2.0 / beatsPerSecond);
            break;
        case Rate::Quarter:
            // One quarter note
            timeRangeInSeconds = static_cast<float>(1.0 / beatsPerSecond);
            break;
        case Rate::Eighth:
            // One eighth note = 0.5 quarter notes
            timeRangeInSeconds = static_cast<float>(0.5 / beatsPerSecond);
            break;
        case Rate::Sixteenth:
            // One sixteenth note = 0.25 quarter notes
            timeRangeInSeconds = static_cast<float>(0.25 / beatsPerSecond);
            break;
        case Rate::ThirtySecond:
            // One thirty-second note = 0.125 quarter notes
            timeRangeInSeconds = static_cast<float>(0.125 / beatsPerSecond);
            break;
    }

    // Update the waveform component's time range
    setTimeRange(timeRangeInSeconds);
}

// Add the implementation for the snap-to-grid methods
void EnvelopeComponent::setSnapToGrid(bool shouldSnap) {
    snapToGridFlag = shouldSnap;
    repaint();
}

void EnvelopeComponent::setGridDivisions(int horizontal, int vertical) {
    horizontalDivisions = horizontal;
    verticalDivisions = vertical;
    repaint();
}

juce::Point<float> EnvelopeComponent::snapToGrid(const juce::Point<float> &point) const {
    if (!snapToGridFlag)
        return point;
    
    // Define threshold distance for snapping (as a fraction of grid cell size)
    const float snapThresholdX = 0.2f / horizontalDivisions;
    const float snapThresholdY = 0.2f / verticalDivisions;
    
    // Calculate grid steps
    float gridStepX = 1.0f / horizontalDivisions;
    float gridStepY = 1.0f / verticalDivisions;
    
    // Initialize result with original point
    float snappedX = point.x;
    float snappedY = point.y;
    
    // Check if point is close to a horizontal grid line
    float xMod = std::fmod(point.x, gridStepX);
    if (xMod < snapThresholdX) {
        // Snap to lower grid line
        snappedX = std::floor(point.x / gridStepX) * gridStepX;
    } else if (gridStepX - xMod < snapThresholdX) {
        // Snap to upper grid line
        snappedX = std::ceil(point.x / gridStepX) * gridStepX;
    }
    
    // Check if point is close to a vertical grid line
    float yMod = std::fmod(point.y, gridStepY);
    if (yMod < snapThresholdY) {
        // Snap to lower grid line
        snappedY = std::floor(point.y / gridStepY) * gridStepY;
    } else if (gridStepY - yMod < snapThresholdY) {
        // Snap to upper grid line
        snappedY = std::ceil(point.y / gridStepY) * gridStepY;
    }
    
    // Ensure values are within 0-1 range
    snappedX = juce::jlimit(0.0f, 1.0f, snappedX);
    snappedY = juce::jlimit(0.0f, 1.0f, snappedY);
    
    return juce::Point<float>(snappedX, snappedY);
}

// Add setupPresetsUI method
void EnvelopeComponent::setupPresetsUI() {
    // Create presets label
    presetShapesLabel = std::make_unique<juce::Label>("presetShapesLabel", "Shape:");
    presetShapesLabel->setFont(juce::Font(14.0f));
    presetShapesLabel->setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(presetShapesLabel.get());
    
    // Create presets combo box
    presetShapesComboBox = std::make_unique<juce::ComboBox>("presetShapesComboBox");
    presetShapesComboBox->addItem("Sine", static_cast<int>(PresetShape::Sine) + 1);
    presetShapesComboBox->addItem("Triangle", static_cast<int>(PresetShape::Triangle) + 1);
    presetShapesComboBox->addItem("Square", static_cast<int>(PresetShape::Square) + 1);
    presetShapesComboBox->addItem("Ramp Up", static_cast<int>(PresetShape::RampUp) + 1);
    presetShapesComboBox->addItem("Ramp Down", static_cast<int>(PresetShape::RampDown) + 1);
    presetShapesComboBox->addItem("Custom", static_cast<int>(PresetShape::Custom) + 1);
    presetShapesComboBox->setSelectedId(static_cast<int>(PresetShape::Custom) + 1);
    presetShapesComboBox->onChange = [this] {
        int selectedId = presetShapesComboBox->getSelectedId();
        if (selectedId > 0) {
            applyPresetShape(static_cast<PresetShape>(selectedId - 1));
        }
    };
    addAndMakeVisible(presetShapesComboBox.get());
}

// Implementing preset shape methods
void EnvelopeComponent::applyPresetShape(PresetShape shape) {
    // Save current shape
    currentPresetShape = shape;
    
    // Apply the selected shape
    switch (shape) {
        case PresetShape::Sine:
            createSineShape(100);
            break;
        case PresetShape::Triangle:
            createTriangleShape();
            break;
        case PresetShape::Square:
            createSquareShape();
            break;
        case PresetShape::RampUp:
            createRampUpShape();
            break;
        case PresetShape::RampDown:
            createRampDownShape();
            break;
        case PresetShape::Custom:
            // Do nothing for custom, it's used when manual edits are made
            break;
    }
}

void EnvelopeComponent::createSineShape(int numPoints) {
    // Clear existing points
    points.clear();
    
    // Create a single cycle sine wave with the specified number of points
    for (int i = 0; i < numPoints; ++i) {
        float x = static_cast<float>(i) / (numPoints - 1); // 0 to 1
        // Sine wave oscillates between 0 and 1 (one complete cycle)
        float y = 0.5f + 0.5f * std::sin(x * 2.0f * juce::MathConstants<float>::pi - juce::MathConstants<float>::halfPi);
        auto point = std::make_unique<EnvelopePoint>(x, y);
        points.push_back(std::move(point));
    }
    
    // Ensure first and last point's x positions are exactly 0 and 1
    if (!points.empty()) {
        points.front()->position.x = 0.0f;
        points.back()->position.x = 1.0f;
        
        // Select first point
        points.front()->selected = true;
    }
    
    // Update parameter mapper
    parameterMapper.setPoints(points);
    notifyPointsChanged();
    repaint();
}

void EnvelopeComponent::createTriangleShape() {
    // Clear existing points
    points.clear();
    
    // Create a triangle wave with 3 points
    points.push_back(std::make_unique<EnvelopePoint>(0.0f, 0.0f));
    points.push_back(std::make_unique<EnvelopePoint>(0.5f, 1.0f));
    points.push_back(std::make_unique<EnvelopePoint>(1.0f, 0.0f));
    
    // Update parameter mapper
    parameterMapper.setPoints(points);
    notifyPointsChanged();
    repaint();
}

void EnvelopeComponent::createSquareShape() {
    // Clear existing points
    points.clear();
    
    // Create a square wave with 4 points
    points.push_back(std::make_unique<EnvelopePoint>(0.0f, 0.0f));
    points.push_back(std::make_unique<EnvelopePoint>(0.0001f, 1.0f)); // Almost 0 for vertical line
    points.push_back(std::make_unique<EnvelopePoint>(0.5f, 1.0f));
    points.push_back(std::make_unique<EnvelopePoint>(0.5001f, 0.0f)); // Just after 0.5 for vertical line
    points.push_back(std::make_unique<EnvelopePoint>(1.0f, 0.0f));
    
    // Update parameter mapper
    parameterMapper.setPoints(points);
    notifyPointsChanged();
    repaint();
}

void EnvelopeComponent::createSawtoothShape() {
    // Clear existing points
    points.clear();
    
    // Create a sawtooth wave with 2 points
    points.push_back(std::make_unique<EnvelopePoint>(0.0f, 1.0f));
    points.push_back(std::make_unique<EnvelopePoint>(1.0f, 0.0f));
    
    // Update parameter mapper
    parameterMapper.setPoints(points);
    notifyPointsChanged();
    repaint();
}

void EnvelopeComponent::createRampUpShape() {
    // Clear existing points
    points.clear();
    
    // Create a ramp up with 2 points
    points.push_back(std::make_unique<EnvelopePoint>(0.0f, 0.0f));
    points.push_back(std::make_unique<EnvelopePoint>(1.0f, 1.0f));
    
    // Update parameter mapper
    parameterMapper.setPoints(points);
    notifyPointsChanged();
    repaint();
}

void EnvelopeComponent::createRampDownShape() {
    // Clear existing points
    points.clear();
    
    // Create a ramp down with 2 points
    points.push_back(std::make_unique<EnvelopePoint>(0.0f, 1.0f));
    points.push_back(std::make_unique<EnvelopePoint>(1.0f, 0.0f));
    
    // Update parameter mapper
    parameterMapper.setPoints(points);
    notifyPointsChanged();
    repaint();
}

// Add setupSnapToGridUI method
void EnvelopeComponent::setupSnapToGridUI() {
    // Create snap-to-grid toggle button
    snapToGridButton = std::make_unique<juce::ToggleButton>("Snap to Grid");
    snapToGridButton->setToggleState(snapToGridFlag, juce::dontSendNotification);
    snapToGridButton->onClick = [this] {
        setSnapToGrid(snapToGridButton->getToggleState());
    };
    addAndMakeVisible(snapToGridButton.get());
}

// Add method to set rate from enum
void EnvelopeComponent::setRateFromEnum(Rate rate) {
    currentRateEnum = rate;
    rateComboBox->setSelectedId(static_cast<int>(rate) + 1, juce::sendNotificationAsync);
}

// Add method to get current rate enum
EnvelopeComponent::Rate EnvelopeComponent::getCurrentRateEnum() const {
    return currentRateEnum;
}