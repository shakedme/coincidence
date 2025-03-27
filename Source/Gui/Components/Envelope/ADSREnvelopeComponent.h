#pragma once

#include "EnvelopeComponent.h"
#include "../../../Shared/ParameterBinding.h"

class ADSREnvelopeComponent : public juce::Component, private juce::Timer {
public:
    struct ADSRParameters {
        float attack = 100.0f;    // in milliseconds
        float decay = 200.0f;     // in milliseconds
        float sustain = 0.5f;     // 0.0 to 1.0
        float release = 200.0f;   // in milliseconds
    };

    explicit ADSREnvelopeComponent(PluginProcessor& p);
    ~ADSREnvelopeComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Mouse interaction
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // Timer callback
    void timerCallback() override;

    // Parameters
    void setAttack(float milliseconds);
    void setDecay(float milliseconds);
    void setSustain(float level);
    void setRelease(float milliseconds);
    
    const ADSRParameters& getParameters() const { return parameters; }

private:
    // Converts ADSR parameters to envelope points
    void updateEnvelopePoints();
    
    // Converts envelope points to ADSR parameters
    void updateParametersFromPoints();
    
    // Updates ADSR parameters from slider values
    void updateEnvelopeFromSliders();
    
    // Draws the envelope and grid
    void drawEnvelope(juce::Graphics& g);
    void drawGrid(juce::Graphics& g);
    void drawTimeMarkers(juce::Graphics& g);

    // Creates and positions ADSR knobs
    void setupKnobs();
    void positionKnobs();

    juce::Point<float> constrainPointPosition(int index, const juce::Point<float>& position) const;

    PluginProcessor& processor;
    ADSRParameters parameters;
    
    // Store envelope points (normalized 0-1 coordinates)
    std::array<juce::Point<float>, 4> envelopePoints;
    
    // The point being dragged (-1 if none)
    int draggedPointIndex = -1;
    
    // The total visible time span in milliseconds
    float visibleTimeSpan = 2000.0f;
    
    // ADSR parameter sliders
    std::unique_ptr<juce::Slider> attackSlider;
    std::unique_ptr<juce::Slider> decaySlider;
    std::unique_ptr<juce::Slider> sustainSlider;
    std::unique_ptr<juce::Slider> releaseSlider;
    
    std::unique_ptr<juce::Label> attackLabel;
    std::unique_ptr<juce::Label> decayLabel;
    std::unique_ptr<juce::Label> sustainLabel;
    std::unique_ptr<juce::Label> releaseLabel;
    
    // Parameter attachments for sliders
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackSliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decaySliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainSliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseSliderAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ADSREnvelopeComponent)
}; 