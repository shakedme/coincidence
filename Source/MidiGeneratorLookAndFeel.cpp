#include "MidiGeneratorProcessor.h"

MidiGeneratorLookAndFeel::MidiGeneratorLookAndFeel()
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

void MidiGeneratorLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, 
                                                float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
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
    else if (slider.getName().startsWith("velocity") || slider.getName().startsWith("density"))
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

    // Draw the main knob body
    g.setColour(knobColor);
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    // Draw the outer ring
    g.setColour(juce::Colours::darkgrey);
    g.drawEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 1.0f);

    // Draw the position indicator arc
    g.setColour(indicatorColor);
    auto arcStartAngle = rotaryStartAngle;

    if (sliderPos > 0.0f)
    {
        // Create a path for the arc
        juce::Path arcPath;
        arcPath.addArc(centreX - arcRadius, centreY - arcRadius,
                       arcRadius * 2.0f, arcRadius * 2.0f,
                       arcStartAngle, toAngle, true);

        // Stroke the path
        g.strokePath(arcPath, juce::PathStrokeType(lineWidth));
    }

    // For gate and velocity sliders, draw a second indicator for the randomized value
    // Find the editor by searching up through the component hierarchy
    if ((slider.getName() == "gate" || slider.getName() == "velocity"))
    {
        // Simplified approach to find the editor
        juce::Component* parent = slider.getParentComponent();
        MidiGeneratorEditor* editor = nullptr;

        // Try to find the editor by traversing up the component tree
        while (parent != nullptr)
        {
            editor = dynamic_cast<MidiGeneratorEditor*>(parent);
            if (editor != nullptr)
                break;

            parent = parent->getParentComponent();
        }

        if (editor != nullptr)
        {
            // Get the processor to check current randomized values
            MidiGeneratorProcessor* processor = dynamic_cast<MidiGeneratorProcessor*>(editor->getAudioProcessor());

            if (processor != nullptr)
            {
                float randomizedValue = 0.0f;
                if (slider.getName() == "gate")
                    randomizedValue = processor->getCurrentRandomizedGate();
                else if (slider.getName() == "velocity")
                    randomizedValue = processor->getCurrentRandomizedVelocity();

                if (randomizedValue > 0.0f)
                {
                    // Calculate randomized angle
                    float randomizedPos = randomizedValue / 100.0f;
                    auto randomizedAngle = rotaryStartAngle + randomizedPos * (rotaryEndAngle - rotaryStartAngle);

                    // Draw a second indicator with lower alpha for the randomized value
                    juce::Colour randomColor = indicatorColor.withAlpha(0.4f);
                    g.setColour(randomColor);

                    // Draw the randomized value as a faded arc
                    juce::Path randomArcPath;
                    randomArcPath.addArc(centreX - arcRadius * 0.8f, centreY - arcRadius * 0.8f,
                                         arcRadius * 1.6f, arcRadius * 1.6f,
                                         arcStartAngle, randomizedAngle, true);

                    g.strokePath(randomArcPath, juce::PathStrokeType(lineWidth * 0.6f));

                    // Draw a small dot at the randomized position
                    auto dotRadius = 2.0f;
                    auto dotCentreX = centreX + (radius * 0.8f) * std::cos(randomizedAngle - juce::MathConstants<float>::halfPi);
                    auto dotCentreY = centreY + (radius * 0.8f) * std::sin(randomizedAngle - juce::MathConstants<float>::halfPi);

                    g.fillEllipse(dotCentreX - dotRadius, dotCentreY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
                }
            }
        }
    }

    // Draw the indicator line
    juce::Path p;
    auto pointerLength = radius * 0.6f;
    auto pointerThickness = 2.0f;

    p.addRectangle(-pointerThickness * 0.5f, -radius + lineWidth, pointerThickness, pointerLength);
    p.applyTransform(juce::AffineTransform::rotation(toAngle).translated(centreX, centreY));
    g.setColour(indicatorColor);
    g.fillPath(p);

    // Draw indicator dot at the end of the line
    auto dotRadius = 3.0f;
    auto dotCentreX = centreX + (radius - lineWidth - pointerLength * 0.5f) * std::cos(toAngle - juce::MathConstants<float>::halfPi);
    auto dotCentreY = centreY + (radius - lineWidth - pointerLength * 0.5f) * std::sin(toAngle - juce::MathConstants<float>::halfPi);

    g.setColour(indicatorColor);
    g.fillEllipse(dotCentreX - dotRadius, dotCentreY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
}

void MidiGeneratorLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                                bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 4.0f;
    auto r = size / 2.0f;
    auto centre = bounds.getCentre();

    // Draw button background
    juce::Path p;
    p.addRoundedRectangle(centre.getX() - r, centre.getY() - r, size, size, size * 0.1f);

    g.setColour(button.getToggleState() ? juce::Colours::lightgrey : juce::Colours::darkgrey);
    g.fillPath(p);

    // Draw outline
    g.setColour(juce::Colours::darkgrey);
    g.drawRoundedRectangle(centre.getX() - r, centre.getY() - r, size, size, size * 0.1f, 1.0f);

    // Draw check mark if toggled
    if (button.getToggleState())
    {
        g.setColour(juce::Colours::black);

        juce::Path check;
        auto thickness = size * 0.15f;

        check.startNewSubPath(centre.getX() - r * 0.4f, centre.getY());
        check.lineTo(centre.getX() - r * 0.1f, centre.getY() + r * 0.4f);
        check.lineTo(centre.getX() + r * 0.5f, centre.getY() - r * 0.4f);

        g.strokePath(check, juce::PathStrokeType(thickness));
    }
}

void MidiGeneratorLookAndFeel::drawTabButton(juce::TabBarButton& button, juce::Graphics& g,
                                             bool isMouseOver, bool isMouseDown)
{
    auto activeArea = button.getActiveArea();
    auto o = button.getTabbedButtonBar().getOrientation();

    auto activeAreaInset = activeArea.reduced(0, isMouseOver ? -1 : 0);
    bool isTabSelected = button.getToggleState();

    const juce::Colour tabBackground(button.findColour(juce::TabbedButtonBar::tabOutlineColourId));
    const juce::Colour textColor = button.findColour(isTabSelected ? juce::TabbedButtonBar::frontTextColourId
                                                                   : juce::TabbedButtonBar::tabTextColourId);

    auto c = textColor;

    // Get the correct colors based on tab name
    if (button.getName().contains("MELODY"))
    {
        g.setColour(isTabSelected ? juce::Colour(0xff52d97d) // Green
                                  : juce::Colours::darkgrey);
    }
    else if (button.getName().contains("RHYTHM"))
    {
        g.setColour(isTabSelected ? juce::Colour(0xff52bfd9) // Blue
                                  : juce::Colours::darkgrey);
    }
    else
    {
        g.setColour(isTabSelected ? juce::Colours::lightgrey
                                  : juce::Colours::darkgrey);
    }
    
    // Draw tab
    if (o == juce::TabbedButtonBar::TabsAtTop)
    {
        activeAreaInset.removeFromBottom(1);
        g.fillRect(activeAreaInset);
        
        if (!isTabSelected)
            g.fillRect(activeAreaInset.removeFromBottom(1).translated(0, 1));
    }
    
    // Draw tab text
    auto textArea = activeArea;
    
    g.setColour(isTabSelected ? juce::Colours::white : juce::Colours::grey);
    g.setFont(juce::Font(14.0f, juce::Font::bold)); // Use a default font instead of button.getFont()
    g.drawFittedText(button.getButtonText(), textArea, juce::Justification::centred, 1);
}