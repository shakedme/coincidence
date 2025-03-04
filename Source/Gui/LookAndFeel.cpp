#include "LookAndFeel.h"
#include "PluginEditor.h"
#include "../Audio/PluginProcessor.h"

LookAndFeel::LookAndFeel()
{
    // Set up color scheme
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff2a2a2a));
    setColour(juce::TabbedComponent::backgroundColourId, juce::Colour(0xff2a2a2a));
    setColour(juce::TabbedComponent::outlineColourId, juce::Colour(0xff3a3a3a));
    setColour(juce::TabbedButtonBar::tabOutlineColourId, juce::Colour(0xff3a3a3a));
    setColour(juce::TabbedButtonBar::frontOutlineColourId, juce::Colour(0xff3a3a3a));

    // Slider colors
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::darkgrey);
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::grey);
    setColour(juce::Slider::thumbColourId, juce::Colours::white);
    setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);

    // Text colors
    setColour(juce::Label::textColourId, juce::Colours::white);

    // Button colors
    setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    setColour(juce::ToggleButton::tickColourId, juce::Colours::lightgrey);

    // ComboBox colors
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff3a3a3a));
    setColour(juce::ComboBox::textColourId, juce::Colours::white);
    setColour(juce::ComboBox::outlineColourId, juce::Colours::darkgrey);
    setColour(juce::ComboBox::buttonColourId, juce::Colours::darkgrey);
    setColour(juce::ComboBox::arrowColourId, juce::Colours::white);
}

void LookAndFeel::drawRotarySlider(juce::Graphics& g,
                                                int x,
                                                int y,
                                                int width,
                                                int height,
                                                float sliderPos,
                                                float rotaryStartAngle,
                                                float rotaryEndAngle,
                                                juce::Slider& slider)
{
    // Determine which color to use based on the slider name
    juce::Colour knobColor;
    juce::Colour indicatorColor;

    if (slider.getName().startsWith("rate"))
    {
        // Rate knobs - cyan/blue
        knobColor = juce::Colour(0xff303030);
        indicatorColor = juce::Colour(0xff52bfd9); // cyan/blue
    }
    else if (slider.getName().startsWith("gate"))
    {
        // Gate knobs - magenta
        knobColor = juce::Colour(0xff303030);
        indicatorColor = juce::Colour(0xffd952bf); // magenta
    }
    else if (slider.getName().startsWith("velocity")
             || slider.getName().startsWith("density"))
    {
        // Velocity and density knobs - orange/amber
        knobColor = juce::Colour(0xff303030);
        indicatorColor = juce::Colour(0xffd9a652); // amber
    }
    else
    {
        // Default - green
        knobColor = juce::Colour(0xff303030);
        indicatorColor = juce::Colour(0xff52d97d); // green
    }

    // Define dimensions
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineWidth = radius * 0.1f;
    auto arcRadius = radius - lineWidth * 0.5f;

    // Center point of knob
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();

    // Draw the metallic knob body
    // Create a metallic gradient for the knob body
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff808080),
                                           centreX - radius * 0.5f,
                                           centreY - radius * 0.5f,
                                           juce::Colour(0xff404040),
                                           centreX + radius,
                                           centreY + radius,
                                           true));
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    // Draw a metallic ring around the knob
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffaaaaaa),
                                           centreX - radius * 0.8f,
                                           centreY - radius * 0.8f,
                                           juce::Colour(0xff333333),
                                           centreX + radius * 0.8f,
                                           centreY + radius * 0.8f,
                                           true));
    g.drawEllipse(centreX - radius * 0.95f,
                  centreY - radius * 0.95f,
                  radius * 1.9f,
                  radius * 1.9f,
                  radius * 0.1f);

    // Add a highlight spot to give a 3D effect
    g.setColour(juce::Colour(0xaaffffff));
    g.fillEllipse(centreX - radius * 0.35f,
                  centreY - radius * 0.35f,
                  radius * 0.25f,
                  radius * 0.25f);

    // Draw the position indicator arc with a subtle bevel effect
    if (sliderPos > 0.0f)
    {
        // Main indicator arc
        g.setColour(indicatorColor);
        juce::Path arcPath;
        arcPath.addArc(centreX - arcRadius,
                       centreY - arcRadius,
                       arcRadius * 2.0f,
                       arcRadius * 2.0f,
                       rotaryStartAngle,
                       toAngle,
                       true);
        g.strokePath(arcPath, juce::PathStrokeType(lineWidth));

        // Add a highlight to the arc for a raised appearance
        g.setColour(indicatorColor.brighter(0.3f));
        juce::Path arcHighlight;
        arcHighlight.addArc(centreX - arcRadius - lineWidth * 0.25f,
                            centreY - arcRadius - lineWidth * 0.25f,
                            arcRadius * 2.0f + lineWidth * 0.5f,
                            arcRadius * 2.0f + lineWidth * 0.5f,
                            rotaryStartAngle,
                            toAngle,
                            true);
        g.strokePath(arcHighlight, juce::PathStrokeType(lineWidth * 0.5f));
    }

    // For gate and velocity sliders, draw a second indicator for the randomized value
    // Find the editor by searching up through the component hierarchy
    if ((slider.getName() == "gate" || slider.getName() == "velocity"))
    {
        // Simplified approach to find processor
        juce::Component* parent = slider.getParentComponent();
        PluginProcessor* processor = nullptr;
        
        // Try to find an editor that contains our processor
        while (parent != nullptr)
        {
            auto* editor = dynamic_cast<PluginEditor*>(parent);
            if (editor != nullptr)
            {
                processor = dynamic_cast<PluginProcessor*>(editor->getAudioProcessor());
                break;
            }
            parent = parent->getParentComponent();
        }

        if (processor != nullptr)
        {
            // Get the current randomized values
            float randomizedValue = 0.0f;
            if (slider.getName() == "gate")
                randomizedValue = processor->getCurrentRandomizedGate();
            else if (slider.getName() == "velocity")
                randomizedValue = processor->getCurrentRandomizedVelocity();

            if (randomizedValue > 0.0f)
            {
                // Calculate randomized angle
                float randomizedPos = randomizedValue / 100.0f;
                auto randomizedAngle =
                    rotaryStartAngle
                    + randomizedPos * (rotaryEndAngle - rotaryStartAngle);

                // Draw a second indicator with lower alpha for the randomized value
                // Use a metallic gradient for the randomized arc
                juce::Colour randomColor = indicatorColor.withAlpha(0.4f);

                // Draw the randomized value as a faded arc with metallic styling
                juce::Path randomArcPath;
                randomArcPath.addArc(centreX - arcRadius * 0.8f,
                                     centreY - arcRadius * 0.8f,
                                     arcRadius * 1.6f,
                                     arcRadius * 1.6f,
                                     rotaryStartAngle,
                                     randomizedAngle,
                                     true);

                g.setColour(randomColor);
                g.strokePath(randomArcPath, juce::PathStrokeType(lineWidth * 0.6f));

                // Add subtle highlight for a 3D effect on the randomized arc
                g.setColour(randomColor.brighter(0.2f).withAlpha(0.3f));
                juce::Path randomArcHighlight;
                randomArcHighlight.addArc(centreX - arcRadius * 0.8f - 1,
                                          centreY - arcRadius * 0.8f - 1,
                                          arcRadius * 1.6f + 2,
                                          arcRadius * 1.6f + 2,
                                          rotaryStartAngle,
                                          randomizedAngle,
                                          true);
                g.strokePath(randomArcHighlight,
                             juce::PathStrokeType(lineWidth * 0.3f));

                // Draw a small metallic dot at the randomized position
                auto dotRadius = 3.0f;
                auto dotCentreX =
                    centreX
                    + (radius * 0.8f)
                          * std::cos(randomizedAngle
                                     - juce::MathConstants<float>::halfPi);
                auto dotCentreY =
                    centreY
                    + (radius * 0.8f)
                          * std::sin(randomizedAngle
                                     - juce::MathConstants<float>::halfPi);

                // Draw the dot with a metallic gradient
                g.setGradientFill(juce::ColourGradient(randomColor.brighter(0.3f),
                                                       dotCentreX - dotRadius / 2,
                                                       dotCentreY - dotRadius / 2,
                                                       randomColor.darker(0.2f),
                                                       dotCentreX + dotRadius,
                                                       dotCentreY + dotRadius,
                                                       true));
                g.fillEllipse(dotCentreX - dotRadius,
                              dotCentreY - dotRadius,
                              dotRadius * 2.0f,
                              dotRadius * 2.0f);

                // Add a tiny highlight to the dot
                g.setColour(juce::Colour(0x80ffffff));
                g.fillEllipse(dotCentreX - dotRadius * 0.4f,
                              dotCentreY - dotRadius * 0.4f,
                              dotRadius * 0.6f,
                              dotRadius * 0.6f);
            }
        }
    }

    // Draw the indicator line with a metallic look
    juce::Path p;
    auto pointerLength = radius * 0.65f;
    auto pointerThickness = 2.5f;

    p.addRoundedRectangle(-pointerThickness * 0.5f,
                          -radius + lineWidth,
                          pointerThickness,
                          pointerLength,
                          1.0f);
    p.applyTransform(
        juce::AffineTransform::rotation(toAngle).translated(centreX, centreY));

    // Draw indicator with slight gradient for 3D effect
    g.setGradientFill(juce::ColourGradient(
        indicatorColor.brighter(0.2f),
        centreX,
        centreY,
        indicatorColor.darker(0.2f),
        centreX + radius * 0.7f * std::cos(toAngle - juce::MathConstants<float>::halfPi),
        centreY + radius * 0.7f * std::sin(toAngle - juce::MathConstants<float>::halfPi),
        false));
    g.fillPath(p);

    // Add a highlight on top of the pointer for 3D effect
    g.setColour(indicatorColor.brighter(0.5f).withAlpha(0.3f));
    juce::Path pHighlight;
    auto hlThickness = pointerThickness * 0.4f;
    pHighlight.addRoundedRectangle(-hlThickness * 0.5f,
                                   -radius + lineWidth,
                                   hlThickness,
                                   pointerLength * 0.7f,
                                   0.5f);
    pHighlight.applyTransform(
        juce::AffineTransform::rotation(toAngle).translated(centreX, centreY));
    g.fillPath(pHighlight);
}

void LookAndFeel::drawScrew(juce::Graphics& g, float x, float y, float size)
{
    const float halfSize = size * 0.5f;
    const float quarterSize = size * 0.25f;

    // Draw screw background
    g.setColour(juce::Colour(0xff5a5a5a));
    g.fillEllipse(x - halfSize, y - halfSize, size, size);

    // Draw metallic highlight
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff8a8a8a),
                                           x - quarterSize,
                                           y - quarterSize,
                                           juce::Colour(0xff3a3a3a),
                                           x + halfSize,
                                           y + halfSize,
                                           true));
    g.fillEllipse(x - halfSize * 0.9f, y - halfSize * 0.9f, size * 0.9f, size * 0.9f);

    // Draw screw slot
    g.setColour(juce::Colour(0xff222222));
    g.drawLine(x - quarterSize, y, x + quarterSize, y, 1.5f);
    g.drawLine(x, y - quarterSize, x, y + quarterSize, 1.5f);
}

void LookAndFeel::drawComboBox(juce::Graphics& g,
                                            int width,
                                            int height,
                                            bool,
                                            int,
                                            int,
                                            int,
                                            int,
                                            juce::ComboBox& box)
{
    auto cornerSize =
        box.findParentComponentOfClass<juce::ChoicePropertyComponent>() != nullptr ? 0.0f
                                                                                   : 3.0f;
    juce::Rectangle<int> boxBounds(0, 0, width, height);

    // Draw metallic background
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff505050), 0, 0, juce::Colour(0xff303030), 0, height, false));
    g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);

    // Draw border
    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);

    // Draw arrow button with raised 3D effect
    juce::Rectangle<int> arrowZone(width - 20, 0, 20, height);

    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff606060),
                                           width - 20,
                                           0,
                                           juce::Colour(0xff404040),
                                           width,
                                           height,
                                           false));
    g.fillRoundedRectangle(arrowZone.toFloat(), cornerSize);

    // Draw highlight on arrow
    g.setColour(juce::Colour(0x30ffffff));
    g.drawLine(width - 20, 2, width - 2, 2, 1.0f);

    // Draw arrow
    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    juce::Path path;
    path.startNewSubPath(width - 15, height * 0.3f);
    path.lineTo(width - 10, height * 0.7f);
    path.lineTo(width - 5, height * 0.3f);
    g.strokePath(path, juce::PathStrokeType(2.0f));
}

// Add custom label styling for an engraved look
void LookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));

    if (!label.isBeingEdited())
    {
        auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        const juce::Font font(getLabelFont(label));

        g.setColour(
            label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha));
        g.setFont(font);

        auto textArea = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());

        g.drawFittedText(
            label.getText(),
            textArea,
            label.getJustificationType(),
            juce::jmax(1,
                       static_cast<int>(static_cast<float>(textArea.getHeight())
                                        / font.getHeight())),
            label.getMinimumHorizontalScale());

        // For section headers, add an engraved effect
        if (font.getHeight() > 18.0f) // Only for larger fonts (section headers)
        {
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.drawFittedText(
                label.getText(),
                textArea.translated(1, 1),
                label.getJustificationType(),
                juce::jmax(1,
                           static_cast<int>(static_cast<float>(textArea.getHeight())
                                            / font.getHeight())),
                label.getMinimumHorizontalScale());
        }
    }
    else if (label.isEnabled())
    {
        g.setColour(label.findColour(juce::Label::outlineColourId));
        g.drawRect(label.getLocalBounds());
    }
}

void LookAndFeel::drawButtonBackground(juce::Graphics& g,
                                       juce::Button& button,
                                       const juce::Colour& backgroundColour,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool shouldDrawButtonAsDown)
{
    auto buttonArea = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
    float cornerSize = 3.0f;

    // Base colors
    juce::Colour baseColor = backgroundColour;

    // Use the component's color if specified, otherwise use a default metal color
    if (baseColor == juce::Colours::white || baseColor == juce::Colour(0x00000000))
        baseColor = juce::Colour(0xff505050);

    // Adjust colors based on state
    juce::Colour topColor, bottomColor, edgeHighlight, edgeShadow;

    if (shouldDrawButtonAsDown)
    {
        // Pressed state - looks pushed in
        topColor = baseColor.darker(0.2f);
        bottomColor = baseColor.darker(0.1f);
        edgeHighlight = juce::Colours::transparentBlack;
        edgeShadow = juce::Colours::white.withAlpha(0.08f);
    }
    else
    {
        // Normal or highlighted state - looks raised
        topColor = shouldDrawButtonAsHighlighted ? baseColor.brighter(0.1f) : baseColor;
        bottomColor = baseColor.darker(0.2f);
        edgeHighlight = juce::Colours::white.withAlpha(0.15f);
        edgeShadow = juce::Colours::black.withAlpha(0.2f);
    }

    // Draw the main button gradient
    g.setGradientFill(juce::ColourGradient(
        topColor, 0.0f, buttonArea.getY(),
        bottomColor, 0.0f, buttonArea.getBottom(),
        false));
    g.fillRoundedRectangle(buttonArea, cornerSize);

    // Draw edge highlight (top and left)
    juce::Path edgeHighlightPath;
    edgeHighlightPath.startNewSubPath(buttonArea.getX() + cornerSize, buttonArea.getY());
    edgeHighlightPath.lineTo(buttonArea.getRight() - cornerSize, buttonArea.getY());
    edgeHighlightPath.addArc(buttonArea.getRight() - cornerSize*2, buttonArea.getY(), cornerSize*2, cornerSize*2, 0.0f, juce::MathConstants<float>::halfPi);
    edgeHighlightPath.lineTo(buttonArea.getRight(), buttonArea.getBottom() - cornerSize);

    if (shouldDrawButtonAsDown)
    {
        // For pressed state, swap the highlight/shadow
        g.setColour(edgeShadow);
    }
    else
    {
        g.setColour(edgeHighlight);
    }
    g.strokePath(edgeHighlightPath, juce::PathStrokeType(1.0f));

    // Draw edge shadow (bottom and right)
    juce::Path edgeShadowPath;
    edgeShadowPath.startNewSubPath(buttonArea.getX() + cornerSize, buttonArea.getBottom());
    edgeShadowPath.lineTo(buttonArea.getRight() - cornerSize, buttonArea.getBottom());
    edgeShadowPath.addArc(buttonArea.getRight() - cornerSize*2, buttonArea.getBottom() - cornerSize*2, cornerSize*2, cornerSize*2, juce::MathConstants<float>::pi, juce::MathConstants<float>::pi + juce::MathConstants<float>::halfPi);
    edgeShadowPath.lineTo(buttonArea.getX() + cornerSize, buttonArea.getY());
    edgeShadowPath.addArc(buttonArea.getX(), buttonArea.getY(), cornerSize*2, cornerSize*2, juce::MathConstants<float>::pi, juce::MathConstants<float>::pi + juce::MathConstants<float>::halfPi);

    if (shouldDrawButtonAsDown)
    {
        // For pressed state, swap the highlight/shadow
        g.setColour(edgeHighlight);
    }
    else
    {
        g.setColour(edgeShadow);
    }
    g.strokePath(edgeShadowPath, juce::PathStrokeType(1.0f));

    // Add a subtle inner metallic shine
    if (!shouldDrawButtonAsDown)
    {
        auto shineArea = buttonArea.reduced(2.0f);
        g.setGradientFill(juce::ColourGradient(
            juce::Colours::white.withAlpha(0.07f), shineArea.getX(), shineArea.getY(),
            juce::Colours::transparentWhite, shineArea.getX(), shineArea.getCentreY(),
            false));
        g.fillRoundedRectangle(shineArea, cornerSize - 1.0f);
    }
    else
    {
        // Add subtle inner shadow for pressed state
        auto shadowArea = buttonArea.reduced(2.0f);
        g.setGradientFill(juce::ColourGradient(
            juce::Colours::black.withAlpha(0.07f), shadowArea.getX(), shadowArea.getY(),
            juce::Colours::transparentBlack, shadowArea.getX(), shadowArea.getCentreY(),
            false));
        g.fillRoundedRectangle(shadowArea, cornerSize - 1.0f);
    }
}