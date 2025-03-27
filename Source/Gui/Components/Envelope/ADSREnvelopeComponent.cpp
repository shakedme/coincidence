#include "ADSREnvelopeComponent.h"
#include "../../../Audio/PluginProcessor.h"

ADSREnvelopeComponent::ADSREnvelopeComponent(PluginProcessor &p)
        : processor(p) {
    updateEnvelopePoints();
    setupKnobs();

    startTimerHz(30);

    setWantsKeyboardFocus(true);
    updateEnvelopeFromSliders();
}

ADSREnvelopeComponent::~ADSREnvelopeComponent() {
    stopTimer();
}

void ADSREnvelopeComponent::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(0xff222222));

    drawGrid(g);
    drawTimeMarkers(g);
    drawEnvelope(g);
}

void ADSREnvelopeComponent::resized() {
    updateEnvelopePoints();
    positionKnobs();
}

void ADSREnvelopeComponent::drawGrid(juce::Graphics &g) {
    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromBottom(60);

    g.setColour(juce::Colour(0xff333333));
    g.fillRect(bounds);

    g.setColour(juce::Colour(0xff444444));

    const int numHorizontalLines = 5;
    for (int i = 0; i <= numHorizontalLines; ++i) {
        float y = bounds.getY() + (i * bounds.getHeight() / numHorizontalLines);
        g.drawLine(bounds.getX(), y, bounds.getRight(), y, 1.0f);
    }

    const int numVerticalLines = 10;
    for (int i = 0; i <= numVerticalLines; ++i) {
        float x = bounds.getX() + (i * bounds.getWidth() / numVerticalLines);
        g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 1.0f);
    }
}

void ADSREnvelopeComponent::drawTimeMarkers(juce::Graphics &g) {
    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromBottom(80);

    g.setColour(juce::Colours::white);
    g.setFont(12.0f);

    const int numMarkers = 5;
    for (int i = 0; i <= numMarkers; ++i) {
        float x = bounds.getX() + (i * bounds.getWidth() / numMarkers);
        if (x == 0) {
            x = 15;
        }
        float timeMs = (i * visibleTimeSpan / numMarkers);
        juce::String timeText;

        if (timeMs >= 1000.0f)
            timeText = juce::String(timeMs / 1000.0f, 1) + "s";
        else
            timeText = juce::String((int) timeMs) + "ms";

        g.drawText(timeText, x - 20, bounds.getBottom() + 2, 40, 20,
                   juce::Justification::centred, false);
    }
}

void ADSREnvelopeComponent::drawEnvelope(juce::Graphics &g) {
    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromBottom(60);

    juce::Path envelopePath;

    juce::Point<float> startPoint = {
            bounds.getX() + envelopePoints[0].x * bounds.getWidth(),
            bounds.getBottom() - envelopePoints[0].y * bounds.getHeight()
    };

    envelopePath.startNewSubPath(startPoint);

    for (int i = 1; i < envelopePoints.size(); ++i) {
        juce::Point<float> point = {
                bounds.getX() + envelopePoints[i].x * bounds.getWidth(),
                bounds.getBottom() - envelopePoints[i].y * bounds.getHeight()
        };
        envelopePath.lineTo(point);
    }

    g.setColour(juce::Colour(0xff00c0ff));
    g.strokePath(envelopePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));

    const float pointSize = 8.0f;
    for (int i = 0; i < envelopePoints.size(); ++i) {
        juce::Point<float> point = {
                bounds.getX() + envelopePoints[i].x * bounds.getWidth(),
                bounds.getBottom() - envelopePoints[i].y * bounds.getHeight()
        };

        g.fillEllipse(point.x - pointSize / 2, point.y - pointSize / 2, pointSize, pointSize);

        if (i == draggedPointIndex) {
            g.setColour(juce::Colours::white);
            g.drawEllipse(point.x - pointSize / 2, point.y - pointSize / 2, pointSize, pointSize, 2.0f);
        }
    }
}

void ADSREnvelopeComponent::setupKnobs() {
    attackSlider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                                  juce::Slider::TextBoxBelow);
    decaySlider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                                 juce::Slider::TextBoxBelow);
    sustainSlider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                                   juce::Slider::TextBoxBelow);
    releaseSlider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                                   juce::Slider::TextBoxBelow);

    attackSlider->setRange(0.0, 1.0, 0.01);
    attackSlider->setTextValueSuffix(" ms");
    attackSlider->setSkewFactorFromMidPoint(0.3);
    attackSlider->setDoubleClickReturnValue(true, 0.1);

    decaySlider->setRange(0.0, 1.0, 0.01);
    decaySlider->setTextValueSuffix(" ms");
    decaySlider->setSkewFactorFromMidPoint(0.3);
    decaySlider->setDoubleClickReturnValue(true, 0.2);

    sustainSlider->setRange(0.0, 1.0, 0.01);
    sustainSlider->setDoubleClickReturnValue(true, 0.5);

    releaseSlider->setRange(0.0, 1.0, 0.01);
    releaseSlider->setTextValueSuffix(" ms");
    releaseSlider->setSkewFactorFromMidPoint(0.3);
    releaseSlider->setDoubleClickReturnValue(true, 0.2);

    attackLabel = std::make_unique<juce::Label>("attackLabel", "A");
    decayLabel = std::make_unique<juce::Label>("decayLabel", "D");
    sustainLabel = std::make_unique<juce::Label>("sustainLabel", "S");
    releaseLabel = std::make_unique<juce::Label>("releaseLabel", "R");

    for (auto *label: {attackLabel.get(), decayLabel.get(), sustainLabel.get(), releaseLabel.get()}) {
        label->setFont(juce::Font(12.0f, juce::Font::bold));
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    }

    addAndMakeVisible(attackSlider.get());
    addAndMakeVisible(decaySlider.get());
    addAndMakeVisible(sustainSlider.get());
    addAndMakeVisible(releaseSlider.get());

    auto &apvts = processor.getAPVTS();

    attackSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, AppState::ID_ADSR_ATTACK, *attackSlider);

    decaySliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, AppState::ID_ADSR_DECAY, *decaySlider);

    sustainSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, AppState::ID_ADSR_SUSTAIN, *sustainSlider);

    releaseSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, AppState::ID_ADSR_RELEASE, *releaseSlider);

    attackSlider->onValueChange = [this] { updateEnvelopeFromSliders(); };
    decaySlider->onValueChange = [this] { updateEnvelopeFromSliders(); };
    sustainSlider->onValueChange = [this] { updateEnvelopeFromSliders(); };
    releaseSlider->onValueChange = [this] { updateEnvelopeFromSliders(); };
}

void ADSREnvelopeComponent::positionKnobs() {
    const int knobSize = 40;
    const int labelHeight = 20;
    const int knobSpacing = 10;

    auto bounds = getLocalBounds();
    auto knobArea = bounds.removeFromBottom(60);

    const int totalWidth = 4 * knobSize + 3 * knobSpacing;
    const int startX = (knobArea.getWidth() - totalWidth) / 2;

    attackLabel->setBounds(startX, knobArea.getY(), knobSize, labelHeight);
    decayLabel->setBounds(startX + knobSize + knobSpacing, knobArea.getY(), knobSize, labelHeight);
    sustainLabel->setBounds(startX + 2 * (knobSize + knobSpacing), knobArea.getY(), knobSize, labelHeight);
    releaseLabel->setBounds(startX + 3 * (knobSize + knobSpacing), knobArea.getY(), knobSize, labelHeight);

    attackSlider->setBounds(startX, knobArea.getY() + labelHeight, knobSize, knobSize);
    decaySlider->setBounds(startX + knobSize + knobSpacing, knobArea.getY() + labelHeight, knobSize, knobSize);
    sustainSlider->setBounds(startX + 2 * (knobSize + knobSpacing), knobArea.getY() + labelHeight, knobSize, knobSize);
    releaseSlider->setBounds(startX + 3 * (knobSize + knobSpacing), knobArea.getY() + labelHeight, knobSize, knobSize);
}

void ADSREnvelopeComponent::updateEnvelopeFromSliders() {
    auto attackNormalized = static_cast<float>(attackSlider->getValue());
    auto decayNormalized = static_cast<float>(decaySlider->getValue());
    auto sustainLevel = static_cast<float>(sustainSlider->getValue());
    auto releaseNormalized = static_cast<float>(releaseSlider->getValue());

    parameters.attack = attackNormalized * 5000.0f;
    parameters.decay = decayNormalized * 5000.0f;
    parameters.sustain = sustainLevel;
    parameters.release = releaseNormalized * 5000.0f;

    updateEnvelopePoints();
}

void ADSREnvelopeComponent::updateEnvelopePoints() {
    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromBottom(60);

    float totalTime = parameters.attack + parameters.decay + parameters.release;

    if (totalTime > visibleTimeSpan) {
        visibleTimeSpan = totalTime * 1.2f;
    }

    float attackX = parameters.attack / visibleTimeSpan;
    float decayX = (parameters.attack + parameters.decay) / visibleTimeSpan;
    float releaseX = decayX + (parameters.release / visibleTimeSpan);

    releaseX = juce::jmin(releaseX, 1.0f);

    envelopePoints[0] = {0.0f, 0.0f};
    envelopePoints[1] = {attackX, 1.0f};
    envelopePoints[2] = {decayX, parameters.sustain};
    envelopePoints[3] = {releaseX, 0.0f};

    repaint();
}

void ADSREnvelopeComponent::updateParametersFromPoints() {
    float attackX = envelopePoints[1].x;
    float decayX = envelopePoints[2].x - envelopePoints[1].x;
    float releaseX = envelopePoints[3].x - envelopePoints[2].x;

    parameters.attack = attackX * visibleTimeSpan;
    parameters.decay = decayX * visibleTimeSpan;
    parameters.sustain = envelopePoints[2].y;
    parameters.release = releaseX * visibleTimeSpan;

    float attackNormalized = parameters.attack / 5000.0f;
    float decayNormalized = parameters.decay / 5000.0f;
    float releaseNormalized = parameters.release / 5000.0f;

    attackSlider->setValue(attackNormalized, juce::sendNotificationAsync);
    decaySlider->setValue(decayNormalized, juce::sendNotificationAsync);
    sustainSlider->setValue(parameters.sustain, juce::sendNotificationAsync);
    releaseSlider->setValue(releaseNormalized, juce::sendNotificationAsync);

    repaint();
}

void ADSREnvelopeComponent::mouseDown(const juce::MouseEvent &e) {
    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromBottom(60);

    draggedPointIndex = -1;

    for (int i = 0; i < envelopePoints.size(); ++i) {
        juce::Point<float> pointPos = {
                bounds.getX() + envelopePoints[i].x * bounds.getWidth(),
                bounds.getBottom() - envelopePoints[i].y * bounds.getHeight()
        };

        if (pointPos.getDistanceFrom(e.position) < 10.0f) {
            draggedPointIndex = i;
            break;
        }
    }
}

void ADSREnvelopeComponent::mouseDrag(const juce::MouseEvent &e) {
    if (draggedPointIndex < 0 || draggedPointIndex >= envelopePoints.size())
        return;

    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromBottom(60);

    juce::Point<float> normalizedPos = {
            (e.position.x - bounds.getX()) / bounds.getWidth(),
            1.0f - (e.position.y - bounds.getY()) / bounds.getHeight()
    };

    normalizedPos = constrainPointPosition(draggedPointIndex, normalizedPos);

    envelopePoints[draggedPointIndex] = normalizedPos;

    updateParametersFromPoints();

    repaint();
}

void ADSREnvelopeComponent::mouseUp(const juce::MouseEvent &e) {
    draggedPointIndex = -1;
}

juce::Point<float> ADSREnvelopeComponent::constrainPointPosition(int index, const juce::Point<float> &position) const {
    juce::Point<float> constrained = position;

    constrained.x = juce::jlimit(0.0f, 1.0f, constrained.x);
    constrained.y = juce::jlimit(0.0f, 1.0f, constrained.y);

    switch (index) {
        case 0:
            return envelopePoints[0];
        case 1:
            constrained.y = 1.0f;
            constrained.x = juce::jlimit(0.01f, envelopePoints[2].x - 0.01f, constrained.x);
            break;
        case 2:
            constrained.x = juce::jlimit(envelopePoints[1].x + 0.01f,
                                         envelopePoints[3].x - 0.01f,
                                         constrained.x);
            break;
        case 3:
            constrained.y = 0.0f;
            constrained.x = juce::jlimit(envelopePoints[2].x + 0.01f, 1.0f, constrained.x);
            break;
    }

    return constrained;
}

void ADSREnvelopeComponent::timerCallback() {
    repaint();
}
