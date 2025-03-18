#include "EnvelopeComponent.h"
#include <juce_graphics/juce_graphics.h>

//==============================================================================
EnvelopeComponent::EnvelopeComponent(TimingManager &tm, EnvelopeParams::ParameterType type)
        : timingManager(tm),
          parameterMapper(type),
          pointManager(),
          renderer(pointManager) {

    // Configure the pointManager
    pointManager.onPointsChanged = [this]() { handlePointsChanged(); };

    setupRateUI();
    setupPresetsUI();

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
    g.fillAll(juce::Colour(0xff222222));

    renderer.drawGrid(g);

    double ppqPosition = timingManager.getPpqPosition();
    float cycle = std::fmod(static_cast<float>(ppqPosition * parameterMapper.getRate()), 1.0f);

    renderer.drawEnvelope(g, cycle);

    if (isCreatingSelectionArea) {
        renderer.drawSelectionArea(g, selectionArea);
    }
}

void EnvelopeComponent::resized() {
    resizeControls(getWidth());

    pointManager.setBounds(getWidth(), getHeight() - removeFromTop);
    renderer.setBounds(getWidth(), getHeight() - removeFromTop);

    const int controlHeight = 25;
    const int padding = 5;
    const int topControlArea = controlHeight + (2 * padding);

    auto waveformBounds = getLocalBounds();
    waveformBounds.removeFromTop(topControlArea);
    waveformComponent.setBounds(waveformBounds);
}

void EnvelopeComponent::mouseDown(const juce::MouseEvent &e) {
    int width = getWidth();
    int height = getHeight() - removeFromTop; // Exclude the control area

    draggedPointIndex = -1;

    // Check if clicking on an existing point
    int pointIndex = pointManager.findPointAt(e.position.toFloat(), 6.0f);
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
        int segmentIndex = pointManager.findClosestSegmentIndex(e.position.toFloat(), 10.0f);
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
    int height = getHeight() - removeFromTop;

    // Check if we're editing a curve
    if (curveEditingSegment >= 0) {
        // Calculate curve adjustment based on vertical drag distance
        // Make the direction intuitive: dragging down creates a downward curve (negative value)
        float verticalDelta = (e.position.getY() - curveEditStartPos.getY()) / 100.0f;

        // Adjust the curvature of the line segment
        pointManager.setCurvature(curveEditingSegment, juce::jlimit(-1.0f, 1.0f, initialCurvature + verticalDelta));

        // Set the UI to Custom preset shape
        setCurrentPresetShape(EnvelopePresetGenerator::PresetShape::Custom);

        repaint();
        return;
    }

    // Handle point dragging using the stored draggedPointIndex
    if (draggedPointIndex >= 0) {
        // Convert screen position to normalized
        float normX = (float) e.x / width;
        float normY = 1.0f - (float) e.y / height; // Invert Y to match the display

        // Set to custom shape since user is modifying points
        setCurrentPresetShape(EnvelopePresetGenerator::PresetShape::Custom);

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
        pointManager.selectPointsInArea(selectionArea);

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
    int height = getHeight() - removeFromTop;

    // First check if double-clicking on an existing point to delete it
    int pointIndex = pointManager.findPointAt(e.position.toFloat(), 6.0f);
    if (pointIndex >= 0) {
        // Try to remove the point (this will fail for first and last points)
        if (pointManager.removePoint(pointIndex)) {

            setCurrentPresetShape(EnvelopePresetGenerator::PresetShape::Custom);
            repaint();
        }
        return;
    }

    // Check if clicking on a curve segment to reset curvature
    int segmentIndex = pointManager.findClosestSegmentIndex(e.position.toFloat(), 10.0f);
    if (segmentIndex >= 0) {
        pointManager.setCurvature(segmentIndex, 0.0f);
        setCurrentPresetShape(EnvelopePresetGenerator::PresetShape::Custom);
        repaint();
        return;
    }

    // If not clicking on a point, create a new one at the double-click position
    float normX = (float) e.x / width;
    float normY = 1.0f - (float) e.y / height; // Invert Y to match the display

    pointManager.deselectAllPoints();
    pointManager.addPoint(normX, normY);

    setCurrentPresetShape(EnvelopePresetGenerator::PresetShape::Custom);

    repaint();
}

bool EnvelopeComponent::keyPressed(const juce::KeyPress &key) {
    // Handle backspace key to delete selected points
    if (key == juce::KeyPress::backspaceKey) {
        if (pointManager.getSelectedPointsCount() > 0) {
            pointManager.clearSelectedPoints();
            setCurrentPresetShape(EnvelopePresetGenerator::PresetShape::Custom);
            repaint();
            return true;
        }
    }
    return false;
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
    updateTimeRangeFromRate();

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
    float timeRangeInSeconds = calculateTimeRangeInSeconds(timingManager.getBpm());
    setTimeRange(timeRangeInSeconds);
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

void EnvelopeComponent::setupRateUI() {
    // Create rate combo box
    rateComboBox = std::make_unique<juce::ComboBox>("rateComboBox");
    rateComboBox->addItem("2/1", static_cast<int>(Rate::TwoWhole) + 1);
    rateComboBox->addItem("1/1", static_cast<int>(Rate::Whole) + 1);
    rateComboBox->addItem("1/2", static_cast<int>(Rate::Half) + 1);
    rateComboBox->addItem("1/4", static_cast<int>(Rate::Quarter) + 1);
    rateComboBox->addItem("1/8", static_cast<int>(Rate::Eighth) + 1);
    rateComboBox->addItem("1/16", static_cast<int>(Rate::Sixteenth) + 1);
    rateComboBox->addItem("1/32", static_cast<int>(Rate::ThirtySecond) + 1);
    rateComboBox->setSelectedId(static_cast<int>(Rate::Quarter) + 1);
    rateComboBox->onChange = [this] {
        updateRateFromComboBox();
    };
    addAndMakeVisible(rateComboBox.get());
}

void EnvelopeComponent::setupPresetsUI() {
    // Create preset shape buttons
    const std::vector<EnvelopePresetGenerator::PresetShape> shapes = {
            EnvelopePresetGenerator::PresetShape::Sine,
            EnvelopePresetGenerator::PresetShape::Triangle,
            EnvelopePresetGenerator::PresetShape::Square,
            EnvelopePresetGenerator::PresetShape::RampUp,
            EnvelopePresetGenerator::PresetShape::RampDown
    };

    const std::vector<juce::String> shapeNames = {
            "Sine", "Triangle", "Square", "Ramp Up", "Ramp Down"
    };

    // Create buttons for each shape
    for (size_t i = 0; i < shapes.size(); ++i) {
        auto button = std::make_unique<EnvelopeShapeButton>(shapeNames[i], shapes[i]);

        // Set up the click callback
        button->onClick = [this, shape = shapes[i]]() {
            handlePresetButtonClick(shape);
        };

        presetButtons.push_back(std::move(button));
        addAndMakeVisible(presetButtons.back().get());
    }
}

void EnvelopeComponent::resizeControls(int width, int topPadding) {
    // Calculate space for the rate controls
    const int controlHeight = 25;
    const int comboWidth = 100;
    const int bottomPadding = 10;
    int bottomEdge = getHeight() - bottomPadding;

    // Position rate controls at the top left
    rateComboBox->setBounds(10, bottomEdge - 20 , comboWidth, controlHeight);

    // Position preset shape buttons in the bottom right corner
    const int buttonSize = 40;
    const int buttonPadding = 5;

    // Calculate the bottom right position
    int rightEdge = width - buttonPadding;

    // Position buttons horizontally
    for (int i = presetButtons.size() - 1; i >= 0; --i) {
        presetButtons[i]->setBounds(rightEdge - buttonSize, bottomEdge - buttonSize, buttonSize, buttonSize);
        rightEdge -= (buttonSize + buttonPadding);
    }
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

    // Set the rate
    setRate(newRate);
}

void EnvelopeComponent::setCurrentPresetShape(EnvelopePresetGenerator::PresetShape shape) {
    currentPresetShape = shape;

    // Update button states - highlight the selected shape
    for (size_t i = 0; i < presetButtons.size(); ++i) {
        auto buttonShape = static_cast<EnvelopePresetGenerator::PresetShape>(i);
        presetButtons[i]->setToggleState(buttonShape == shape, juce::dontSendNotification);
    }

    repaint();
}

float EnvelopeComponent::calculateTimeRangeInSeconds(double bpm) const {
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

    return timeRangeInSeconds;
}

void EnvelopeComponent::handlePresetButtonClick(EnvelopePresetGenerator::PresetShape shape) {
    currentPresetShape = shape;
    auto newPoints = EnvelopePresetGenerator::createShape(shape);
    pointManager.setPoints(std::move(newPoints));
    repaint();
}