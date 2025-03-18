#include "EnvelopeComponent.h"
#include <juce_graphics/juce_graphics.h>

//==============================================================================
EnvelopeComponent::EnvelopeComponent(TimingManager &tm, EnvelopeParams::ParameterType type)
        : timingManager(tm),
          parameterMapper(type),
          pointManager(gridSystem),
          renderer(pointManager),
          uiControlsManager() {

    // Configure the pointManager
    pointManager.onPointsChanged = [this]() { handlePointsChanged(); };

    // Configure the uiControlsManager
    uiControlsManager.onRateChanged = [this](float rate) { handleRateChanged(rate); };
    uiControlsManager.onPresetShapeChanged = [this](
            EnvelopePresetGenerator::PresetShape shape) { handlePresetShapeChanged(shape); };
    uiControlsManager.onSnapToGridChanged = [this](bool enabled) { handleSnapToGridChanged(enabled); };

    // Setup UI controls
    uiControlsManager.setupControls(this);

    // Add the waveform component as a child component
    addAndMakeVisible(waveformComponent);

    // Configure the waveform component to be transparent so we can draw over it
    waveformComponent.setBackgroundColour(juce::Colours::transparentBlack);
    waveformComponent.setWaveformAlpha(0.3f);
    waveformComponent.setWaveformColour(juce::Colour(0xff52bfd9));
    waveformComponent.setWaveformScaleFactor(1.0f);

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
    gridSystem.drawGrid(g, getWidth(), getHeight(), gridSystem.isSnapToGridEnabled());

    // Calculate current transport position
    double ppqPosition = timingManager.getPpqPosition();
    float cycle = std::fmod(static_cast<float>(ppqPosition * parameterMapper.getRate()), 1.0f);

    // Draw envelope (includes line, points, and position marker)
    renderer.drawEnvelope(g, getWidth(), getHeight(), cycle);

    // Draw selection area if active
    if (isCreatingSelectionArea) {
        renderer.drawSelectionArea(g, selectionArea);
    }
}

void EnvelopeComponent::resized() {
    // Resize UI controls
    uiControlsManager.resizeControls(getWidth());

    // Calculate space for the UI controls
    const int controlHeight = 25;
    const int padding = 5;
    const int topControlArea = controlHeight + (2 * padding);

    // Position the waveform component below the controls
    auto waveformBounds = getLocalBounds();
    waveformBounds.removeFromTop(topControlArea);
    waveformComponent.setBounds(waveformBounds);
}

void EnvelopeComponent::mouseDown(const juce::MouseEvent &e) {
    // Get component dimensions
    int width = getWidth();
    int height = getHeight();

    // Reset drag tracking
    draggedPointIndex = -1;

    // Check if clicking on an existing point
    int pointIndex = pointManager.findPointAt(e.position.toFloat(), 6.0f, width, height);
    if (pointIndex >= 0) {
        // If clicking on an already selected point, don't clear other selections
        // unless shift is not held
        if (!pointManager.getPoints()[pointIndex]->selected && !e.mods.isShiftDown()) {
            // Clear previous selection if clicking on an unselected point
            pointManager.deselectAllPoints();
        }

        pointManager.selectPoint(pointIndex);
        draggedPointIndex = pointIndex; // Store the point index for dragging
        repaint();
        return;
    }

    // Check if Alt is held and clicking on a line segment to adjust curvature
    if (e.mods.isAltDown()) {
        // Find the line segment closest to the click point
        int segmentIndex = pointManager.findClosestSegmentIndex(e.position.toFloat(), 10.0f, width, height);
        if (segmentIndex >= 0) {
            curveEditingSegment = segmentIndex;
            // Start with current curvature value
            initialCurvature = pointManager.getCurvature(segmentIndex);
            curveEditStartPos = e.position;
            repaint();
            return;
        }
    }

    // If not clicking on a point or line segment, start creating a selection area
    if (!e.mods.isAltDown()) {
        // Clear current selection if not adding to it
        if (!e.mods.isShiftDown()) {
            pointManager.deselectAllPoints();
        }

        isCreatingSelectionArea = true;
        selectionStart = e.position;
        selectionArea.setPosition(selectionStart);
        selectionArea.setSize(0, 0);
        repaint();
    }
}

void EnvelopeComponent::mouseDrag(const juce::MouseEvent &e) {
    // Get component dimensions
    int width = getWidth();
    int height = getHeight();

    // Check if we're editing a curve
    if (curveEditingSegment >= 0) {
        // Calculate curve adjustment based on vertical drag distance
        // Make the direction intuitive: dragging down creates a downward curve (negative value)
        float verticalDelta = (e.position.getY() - curveEditStartPos.getY()) / 100.0f;

        // Adjust the curvature of the line segment
        pointManager.setCurvature(curveEditingSegment, juce::jlimit(-1.0f, 1.0f, initialCurvature + verticalDelta));

        // Set the UI to Custom preset shape
        uiControlsManager.setCurrentPresetShape(EnvelopePresetGenerator::PresetShape::Custom);

        repaint();
        return;
    }

    // Handle point dragging using the stored draggedPointIndex
    if (draggedPointIndex >= 0) {
        // Convert screen position to normalized
        float normX = (float) e.x / width;
        float normY = 1.0f - (float) e.y / height; // Invert Y to match the display

        // Set to custom shape since user is modifying points
        uiControlsManager.setCurrentPresetShape(EnvelopePresetGenerator::PresetShape::Custom);

        // Move the point
        pointManager.movePoint(draggedPointIndex, normX, normY);

        repaint();
        return;
    }

    // Handle selection area creation
    if (isCreatingSelectionArea) {
        // Update the selection rectangle
        float left = std::min(selectionStart.x, e.position.x);
        float top = std::min(selectionStart.y, e.position.y);
        float right = std::max(selectionStart.x, e.position.x);
        float bottom = std::max(selectionStart.y, e.position.y);

        selectionArea.setBounds(left, top, right - left, bottom - top);

        // Select points within the area as we drag
        pointManager.selectPointsInArea(selectionArea, width, height);

        repaint();
    }

}

void EnvelopeComponent::mouseUp(const juce::MouseEvent &) {
    curveEditingSegment = -1;
    draggedPointIndex = -1; // Reset the dragged point index

    // Finalize the selection area
    if (isCreatingSelectionArea) {
        isCreatingSelectionArea = false;

        // If the selection area is very small, it might have been an accidental click
        // In that case, clear the selection
        if (selectionArea.getWidth() < 5 && selectionArea.getHeight() < 5) {
            pointManager.deselectAllPoints();
        }

        repaint();
    }
}

void EnvelopeComponent::mouseDoubleClick(const juce::MouseEvent &e) {
    // Get component dimensions
    int width = getWidth();
    int height = getHeight();

    // First check if double-clicking on an existing point to delete it
    int pointIndex = pointManager.findPointAt(e.position.toFloat(), 6.0f, width, height);
    if (pointIndex >= 0) {
        // Try to remove the point (this will fail for first and last points)
        if (pointManager.removePoint(pointIndex)) {
            // Set the current preset shape to Custom since points changed
            uiControlsManager.setCurrentPresetShape(EnvelopePresetGenerator::PresetShape::Custom);
            repaint();
        }
        return;
    }

    // Check if clicking on a curve segment to reset curvature
    int segmentIndex = pointManager.findClosestSegmentIndex(e.position.toFloat(), 10.0f, width, height);
    if (segmentIndex >= 0) {
        pointManager.setCurvature(segmentIndex, 0.0f);

        // Set the current preset shape to Custom since curves changed
        uiControlsManager.setCurrentPresetShape(EnvelopePresetGenerator::PresetShape::Custom);

        repaint();
        return;
    }

    // If not clicking on a point, create a new one at the double-click position
    float normX = (float) e.x / width;
    float normY = 1.0f - (float) e.y / height; // Invert Y to match the display

    pointManager.deselectAllPoints();
    pointManager.addPoint(normX, normY);

    // Set the current preset shape to Custom since user added a point
    uiControlsManager.setCurrentPresetShape(EnvelopePresetGenerator::PresetShape::Custom);

    repaint();
}

bool EnvelopeComponent::keyPressed(const juce::KeyPress &key) {
    // Handle backspace key to delete selected points
    if (key == juce::KeyPress::backspaceKey) {
        if (pointManager.getSelectedPointsCount() > 0) {
            // This will only remove non-fixed points
            pointManager.clearSelectedPoints();

            // Set the current preset shape to Custom
            uiControlsManager.setCurrentPresetShape(EnvelopePresetGenerator::PresetShape::Custom);

            repaint();
            return true;
        }
    }
    return false;
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

void EnvelopeComponent::setParameterRange(float min, float max, bool exponential) {
    parameterMapper.setParameterRange(min, max, exponential);
}

float EnvelopeComponent::getCurrentValue() const {
    return parameterMapper.getCurrentValue();
}

void EnvelopeComponent::setRate(float newRate) {
    parameterMapper.setRate(newRate);

    // Update UI control manager
    uiControlsManager.setRate(newRate);

    // Update time range
    updateTimeRangeFromRate();

    // Notify listeners of rate change if callback is set
    if (onRateChanged) {
        onRateChanged(newRate);
    }
}

void EnvelopeComponent::setParameterType(EnvelopeParams::ParameterType type) {
    parameterMapper.setParameterType(type);
    repaint();
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

void EnvelopeComponent::updateTimeRangeFromRate() {
    float timeRangeInSeconds = uiControlsManager.calculateTimeRangeInSeconds(timingManager.getBpm());
    setTimeRange(timeRangeInSeconds);
}

void EnvelopeComponent::setSnapToGrid(bool shouldSnap) {
    gridSystem.setSnapToGridEnabled(shouldSnap);
    uiControlsManager.setSnapToGridEnabled(shouldSnap);
    repaint();
}

const std::vector<std::unique_ptr<EnvelopePoint>> &EnvelopeComponent::getPoints() const {
    return pointManager.getPoints();
}

void EnvelopeComponent::handlePointsChanged() {
    // Update parameter mapper with new points
    parameterMapper.setPoints(pointManager.getPoints());

    if (onPointsChanged) {
        onPointsChanged(pointManager.getPoints());
    }
    // Request repaint
    repaint();
}

void EnvelopeComponent::handlePresetShapeChanged(EnvelopePresetGenerator::PresetShape shape) {
    applyPresetShape(shape);
}

void EnvelopeComponent::handleSnapToGridChanged(bool enabled) {
    setSnapToGrid(enabled);
}

void EnvelopeComponent::handleRateChanged(float rate) {
    parameterMapper.setRate(rate);
    updateTimeRangeFromRate();

    // Notify listeners
    if (onRateChanged) {
        onRateChanged(rate);
    }
}

void EnvelopeComponent::applyPresetShape(EnvelopePresetGenerator::PresetShape shape) {
    auto newPoints = EnvelopePresetGenerator::createShape(shape);
    pointManager.setPoints(std::move(newPoints));
    repaint();
}