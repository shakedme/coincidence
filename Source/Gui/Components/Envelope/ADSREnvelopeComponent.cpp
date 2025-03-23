#include "ADSREnvelopeComponent.h"
#include "../../../Audio/PluginProcessor.h"

ADSREnvelopeComponent::ADSREnvelopeComponent(PluginProcessor& p)
    : processor(p)
{
    // Initialize envelope points (in normalized coordinates)
    // Point 0: Initial point (0,0)
    // Point 1: Attack peak (attack_time, 1.0)
    // Point 2: Sustain level (attack_time + decay_time, sustain_level)
    // Point 3: End point (attack_time + decay_time + release_time, 0)
    updateEnvelopePoints();
    
    // Set up UI elements
    setupKnobs();
    
    // Create parameter attachments for the sliders
    auto& apvts = processor.getAPVTS();
    
    attackSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, AppState::ID_ADSR_ATTACK, *attackSlider);
    
    decaySliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, AppState::ID_ADSR_DECAY, *decaySlider);
    
    sustainSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, AppState::ID_ADSR_SUSTAIN, *sustainSlider);
    
    releaseSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, AppState::ID_ADSR_RELEASE, *releaseSlider);
    
    // Add value listener to update the envelope display when parameters change
    attackSlider->onValueChange = [this] { updateEnvelopeFromSliders(); };
    decaySlider->onValueChange = [this] { updateEnvelopeFromSliders(); };
    sustainSlider->onValueChange = [this] { updateEnvelopeFromSliders(); };
    releaseSlider->onValueChange = [this] { updateEnvelopeFromSliders(); };

    // Start timer for updates
    startTimerHz(30);
    
    setWantsKeyboardFocus(true);
}

ADSREnvelopeComponent::~ADSREnvelopeComponent()
{
    stopTimer();
}

void ADSREnvelopeComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff222222));
    
    drawGrid(g);
    drawTimeMarkers(g);
    drawEnvelope(g);
}

void ADSREnvelopeComponent::resized()
{
    // Update envelope points for new size
    updateEnvelopePoints();
    
    // Position the knobs
    positionKnobs();
}

void ADSREnvelopeComponent::drawGrid(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromBottom(60); // Space for knobs
    
    // Draw background
    g.setColour(juce::Colour(0xff333333));
    g.fillRect(bounds);
    
    // Draw grid lines
    g.setColour(juce::Colour(0xff444444));
    
    // Horizontal lines (amplitude)
    const int numHorizontalLines = 5;
    for (int i = 0; i <= numHorizontalLines; ++i)
    {
        float y = bounds.getY() + (i * bounds.getHeight() / numHorizontalLines);
        g.drawLine(bounds.getX(), y, bounds.getRight(), y, 1.0f);
    }
    
    // Vertical lines (time)
    const int numVerticalLines = 10;
    for (int i = 0; i <= numVerticalLines; ++i)
    {
        float x = bounds.getX() + (i * bounds.getWidth() / numVerticalLines);
        g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 1.0f);
    }
}

void ADSREnvelopeComponent::drawTimeMarkers(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromBottom(80); // Space for knobs
    
    // Draw time markers
    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    
    const int numMarkers = 5;
    for (int i = 0; i <= numMarkers; ++i)
    {
        float x = bounds.getX() + (i * bounds.getWidth() / numMarkers);
        float timeMs = (i * visibleTimeSpan / numMarkers);
        juce::String timeText;
        
        if (timeMs >= 1000.0f)
            timeText = juce::String(timeMs / 1000.0f, 1) + "s";
        else
            timeText = juce::String((int)timeMs) + "ms";
            
        g.drawText(timeText, x - 20, bounds.getBottom() + 2, 40, 20, 
                  juce::Justification::centred, false);
    }
}

void ADSREnvelopeComponent::drawEnvelope(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromBottom(60); // Space for knobs
    
    // Create path for the envelope
    juce::Path envelopePath;
    
    // Convert normalized points to screen coordinates
    juce::Point<float> startPoint = {
        bounds.getX() + envelopePoints[0].x * bounds.getWidth(),
        bounds.getBottom() - envelopePoints[0].y * bounds.getHeight()
    };
    
    envelopePath.startNewSubPath(startPoint);
    
    // Add lines to the remaining points
    for (int i = 1; i < envelopePoints.size(); ++i)
    {
        juce::Point<float> point = {
            bounds.getX() + envelopePoints[i].x * bounds.getWidth(),
            bounds.getBottom() - envelopePoints[i].y * bounds.getHeight()
        };
        envelopePath.lineTo(point);
    }
    
    // Draw the envelope path
    g.setColour(juce::Colour(0xff00c0ff));
    g.strokePath(envelopePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));
    
    // Draw points
    const float pointSize = 8.0f;
    for (int i = 0; i < envelopePoints.size(); ++i)
    {
        juce::Point<float> point = {
            bounds.getX() + envelopePoints[i].x * bounds.getWidth(),
            bounds.getBottom() - envelopePoints[i].y * bounds.getHeight()
        };
        
        // Different colors for different points
        switch (i) {
            case 0: g.setColour(juce::Colours::grey); break;      // Start
            case 1: g.setColour(juce::Colours::orange); break;    // Attack
            case 2: g.setColour(juce::Colours::green); break;     // Decay/Sustain
            case 3: g.setColour(juce::Colours::red); break;       // Release
        }
        
        g.fillEllipse(point.x - pointSize/2, point.y - pointSize/2, pointSize, pointSize);
        
        // Highlight if being dragged
        if (i == draggedPointIndex)
        {
            g.setColour(juce::Colours::white);
            g.drawEllipse(point.x - pointSize/2, point.y - pointSize/2, pointSize, pointSize, 2.0f);
        }
    }
}

void ADSREnvelopeComponent::setupKnobs()
{
    // Create sliders for ADSR controls
    attackSlider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, 
                                                juce::Slider::TextBoxBelow);
    decaySlider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, 
                                                juce::Slider::TextBoxBelow);
    sustainSlider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, 
                                                 juce::Slider::TextBoxBelow);
    releaseSlider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, 
                                                 juce::Slider::TextBoxBelow);
    
    // Configure sliders
    attackSlider->setRange(0.0, 1.0, 0.01);
    attackSlider->setTextValueSuffix(" ms");
    attackSlider->setSkewFactorFromMidPoint(0.3); // Make the slider more precise at lower values
    attackSlider->setDoubleClickReturnValue(true, 0.1); // Default to 100ms
    
    decaySlider->setRange(0.0, 1.0, 0.01);
    decaySlider->setTextValueSuffix(" ms");
    decaySlider->setSkewFactorFromMidPoint(0.3); // Make the slider more precise at lower values
    decaySlider->setDoubleClickReturnValue(true, 0.2); // Default to 200ms
    
    sustainSlider->setRange(0.0, 1.0, 0.01);
    sustainSlider->setDoubleClickReturnValue(true, 0.5); // Default to 0.5
    
    releaseSlider->setRange(0.0, 1.0, 0.01);
    releaseSlider->setTextValueSuffix(" ms");
    releaseSlider->setSkewFactorFromMidPoint(0.3); // Make the slider more precise at lower values
    releaseSlider->setDoubleClickReturnValue(true, 0.2); // Default to 200ms
    
    // Create labels
    attackLabel = std::make_unique<juce::Label>("attackLabel", "A");
    decayLabel = std::make_unique<juce::Label>("decayLabel", "D");
    sustainLabel = std::make_unique<juce::Label>("sustainLabel", "S");
    releaseLabel = std::make_unique<juce::Label>("releaseLabel", "R");
    
    // Configure labels
    for (auto* label : {attackLabel.get(), decayLabel.get(), sustainLabel.get(), releaseLabel.get()})
    {
        label->setFont(juce::Font(12.0f, juce::Font::bold));
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    }
    
    // Add sliders to the component
    addAndMakeVisible(attackSlider.get());
    addAndMakeVisible(decaySlider.get());
    addAndMakeVisible(sustainSlider.get());
    addAndMakeVisible(releaseSlider.get());
}

void ADSREnvelopeComponent::positionKnobs()
{
    const int knobSize = 40;
    const int labelHeight = 20;
    const int knobSpacing = 10;
    
    auto bounds = getLocalBounds();
    auto knobArea = bounds.removeFromBottom(60);
    
    // Calculate positions for the knobs
    const int totalWidth = 4 * knobSize + 3 * knobSpacing;
    const int startX = (knobArea.getWidth() - totalWidth) / 2;
    
    // Position labels above sliders
    attackLabel->setBounds(startX, knobArea.getY(), knobSize, labelHeight);
    decayLabel->setBounds(startX + knobSize + knobSpacing, knobArea.getY(), knobSize, labelHeight);
    sustainLabel->setBounds(startX + 2 * (knobSize + knobSpacing), knobArea.getY(), knobSize, labelHeight);
    releaseLabel->setBounds(startX + 3 * (knobSize + knobSpacing), knobArea.getY(), knobSize, labelHeight);
    
    // Position sliders
    attackSlider->setBounds(startX, knobArea.getY() + labelHeight, knobSize, knobSize);
    decaySlider->setBounds(startX + knobSize + knobSpacing, knobArea.getY() + labelHeight, knobSize, knobSize);
    sustainSlider->setBounds(startX + 2 * (knobSize + knobSpacing), knobArea.getY() + labelHeight, knobSize, knobSize);
    releaseSlider->setBounds(startX + 3 * (knobSize + knobSpacing), knobArea.getY() + labelHeight, knobSize, knobSize);
}

void ADSREnvelopeComponent::updateEnvelopeFromSliders()
{
    // Get values from sliders
    float attackNormalized = static_cast<float>(attackSlider->getValue());
    float decayNormalized = static_cast<float>(decaySlider->getValue());
    float sustainLevel = static_cast<float>(sustainSlider->getValue());
    float releaseNormalized = static_cast<float>(releaseSlider->getValue());
    
    // Convert normalized values to milliseconds
    parameters.attack = attackNormalized * 5000.0f;
    parameters.decay = decayNormalized * 5000.0f;
    parameters.sustain = sustainLevel;
    parameters.release = releaseNormalized * 5000.0f;
    
    // Update envelope display
    updateEnvelopePoints();
}

void ADSREnvelopeComponent::updateEnvelopePoints()
{
    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromBottom(60); // Space for knobs
    
    // Convert ADSR parameters to normalized time points
    float totalTime = parameters.attack + parameters.decay + parameters.release;
    
    // Normalize to fit in visible time span
    if (totalTime > visibleTimeSpan)
    {
        visibleTimeSpan = totalTime * 1.2f; // Add some margin
    }
    
    // Calculate normalized points
    float attackX = parameters.attack / visibleTimeSpan;
    float decayX = (parameters.attack + parameters.decay) / visibleTimeSpan;
    float releaseX = decayX + (parameters.release / visibleTimeSpan);
    
    // Ensure points stay within bounds
    releaseX = juce::jmin(releaseX, 1.0f);
    
    // Update points array
    envelopePoints[0] = { 0.0f, 0.0f };                   // Start point
    envelopePoints[1] = { attackX, 1.0f };                // Attack peak
    envelopePoints[2] = { decayX, parameters.sustain };   // Sustain level
    envelopePoints[3] = { releaseX, 0.0f };               // End point
    
    repaint();
}

void ADSREnvelopeComponent::updateParametersFromPoints()
{
    // Convert normalized envelope points back to ADSR parameters
    float attackX = envelopePoints[1].x;
    float decayX = envelopePoints[2].x - envelopePoints[1].x;
    float releaseX = envelopePoints[3].x - envelopePoints[2].x;
    
    // Convert normalized times to milliseconds
    parameters.attack = attackX * visibleTimeSpan;
    parameters.decay = decayX * visibleTimeSpan;
    parameters.sustain = envelopePoints[2].y;
    parameters.release = releaseX * visibleTimeSpan;
    
    // Convert to normalized values for sliders (0-1 range)
    float attackNormalized = parameters.attack / 5000.0f;
    float decayNormalized = parameters.decay / 5000.0f;
    float releaseNormalized = parameters.release / 5000.0f;
    
    // Update sliders without triggering callbacks
    attackSlider->setValue(attackNormalized, juce::dontSendNotification);
    decaySlider->setValue(decayNormalized, juce::dontSendNotification);
    sustainSlider->setValue(parameters.sustain, juce::dontSendNotification);
    releaseSlider->setValue(releaseNormalized, juce::dontSendNotification);
    
    repaint();
}

void ADSREnvelopeComponent::mouseDown(const juce::MouseEvent& e)
{
    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromBottom(60); // Space for knobs
    
    draggedPointIndex = -1;
    
    // Check which point was clicked
    for (int i = 0; i < envelopePoints.size(); ++i)
    {
        juce::Point<float> pointPos = {
            bounds.getX() + envelopePoints[i].x * bounds.getWidth(),
            bounds.getBottom() - envelopePoints[i].y * bounds.getHeight()
        };
        
        if (pointPos.getDistanceFrom(e.position) < 10.0f)
        {
            draggedPointIndex = i;
            break;
        }
    }
}

void ADSREnvelopeComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (draggedPointIndex < 0 || draggedPointIndex >= envelopePoints.size())
        return;
    
    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromBottom(60); // Space for knobs
    
    // Convert screen position to normalized position
    juce::Point<float> normalizedPos = {
        (e.position.x - bounds.getX()) / bounds.getWidth(),
        1.0f - (e.position.y - bounds.getY()) / bounds.getHeight() // Invert Y
    };
    
    // Apply constraints based on which point is being dragged
    normalizedPos = constrainPointPosition(draggedPointIndex, normalizedPos);
    
    // Update the point position
    envelopePoints[draggedPointIndex] = normalizedPos;
    
    // Update parameters based on new points
    updateParametersFromPoints();
    
    repaint();
}

void ADSREnvelopeComponent::mouseUp(const juce::MouseEvent& e)
{
    draggedPointIndex = -1;
}

bool ADSREnvelopeComponent::canPointMove(int index, const juce::Point<float>& newPosition) const
{
    // Apply constraints based on point index
    switch (index)
    {
        case 0: // Start point - cannot move
            return false;
            
        case 1: // Attack point - can only move horizontally
            return newPosition.x > 0.0f && newPosition.x < envelopePoints[2].x;
            
        case 2: // Decay/Sustain point - constrained between attack and release
            return newPosition.x > envelopePoints[1].x && newPosition.x < envelopePoints[3].x;
            
        case 3: // Release point - can only move horizontally
            return newPosition.x > envelopePoints[2].x && newPosition.x <= 1.0f;
            
        default:
            return false;
    }
}

juce::Point<float> ADSREnvelopeComponent::constrainPointPosition(int index, const juce::Point<float>& position) const
{
    juce::Point<float> constrained = position;
    
    // Clamp values to 0-1 range
    constrained.x = juce::jlimit(0.0f, 1.0f, constrained.x);
    constrained.y = juce::jlimit(0.0f, 1.0f, constrained.y);
    
    // Apply specific constraints for each point
    switch (index)
    {
        case 0: // Start point - fixed position
            return envelopePoints[0];
            
        case 1: // Attack point - fixed Y at 1.0, X constrained between 0 and decay
            constrained.y = 1.0f;
            constrained.x = juce::jlimit(0.01f, envelopePoints[2].x - 0.01f, constrained.x);
            break;
            
        case 2: // Decay/Sustain point - constrained between attack and release
            constrained.x = juce::jlimit(envelopePoints[1].x + 0.01f, 
                                        envelopePoints[3].x - 0.01f, 
                                        constrained.x);
            break;
            
        case 3: // Release point - fixed Y at 0.0, X constrained between decay and 1.0
            constrained.y = 0.0f;
            constrained.x = juce::jlimit(envelopePoints[2].x + 0.01f, 1.0f, constrained.x);
            break;
    }
    
    return constrained;
}

void ADSREnvelopeComponent::timerCallback()
{
    auto& tree = processor.getAPVTS().state;
    
    // Update amplitude envelope parameter (for legacy compatibility)
    tree.setProperty(AppState::ID_AMPLITUDE_ENVELOPE, parameters.sustain, nullptr);
    
    repaint();
}

void ADSREnvelopeComponent::setAttack(float milliseconds)
{
    parameters.attack = milliseconds;
    updateEnvelopePoints();
}

void ADSREnvelopeComponent::setDecay(float milliseconds)
{
    parameters.decay = milliseconds;
    updateEnvelopePoints();
}

void ADSREnvelopeComponent::setSustain(float level)
{
    parameters.sustain = level;
    updateEnvelopePoints();
}

void ADSREnvelopeComponent::setRelease(float milliseconds)
{
    parameters.release = milliseconds;
    updateEnvelopePoints();
}

juce::Point<float> ADSREnvelopeComponent::getPointPosition(int index) const
{
    if (index < 0 || index >= envelopePoints.size())
        return { 0.0f, 0.0f };
        
    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromBottom(60); // Space for knobs
    
    return {
        bounds.getX() + envelopePoints[index].x * bounds.getWidth(),
        bounds.getBottom() - envelopePoints[index].y * bounds.getHeight()
    };
} 