#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "EnvelopePresetGenerator.h"

/**
 * Manages UI controls for the envelope component (rate, presets, snap-to-grid)
 */
class EnvelopeUIControlsManager {
public:
    // Rate enum for time signature divisions
    enum class Rate {
        TwoWhole = 0,
        Whole,
        Half,
        Quarter,     // 1/4 note - 1 beat
        Eighth,      // 1/8 note - 1/2 beat
        Sixteenth,   // 1/16 note - 1/4 beat
        ThirtySecond // 1/32 note - 1/8 beat
    };

    EnvelopeUIControlsManager();
    ~EnvelopeUIControlsManager() = default;
    
    // Setup UI controls
    void setupControls(juce::Component* parent);
    
    // UI layout
    void resizeControls(int width, int topPadding = 5);
    
    // Rate management
    void setRate(float rate);
    void updateRateFromComboBox();
    
    // Preset shape management
    void setCurrentPresetShape(EnvelopePresetGenerator::PresetShape shape);

    // Snap-to-grid management
    void setSnapToGridEnabled(bool enabled);

    // Callbacks
    std::function<void(float)> onRateChanged;
    std::function<void(EnvelopePresetGenerator::PresetShape)> onPresetShapeChanged;
    std::function<void(bool)> onSnapToGridChanged;
    
    // Calculate time range in seconds based on BPM and rate
    float calculateTimeRangeInSeconds(double bpm) const;
    
private:
    // Setup individual control groups
    void setupRateUI(juce::Component* parent);
    void setupPresetsUI(juce::Component* parent);
    void setupSnapToGridUI(juce::Component* parent);
    
    // UI components
    std::unique_ptr<juce::ComboBox> rateComboBox;
    std::unique_ptr<juce::Label> rateLabel;
    Rate currentRateEnum = Rate::Quarter;
    float currentRate = 1.0f;
    
    std::unique_ptr<juce::ComboBox> presetShapesComboBox;
    std::unique_ptr<juce::Label> presetShapesLabel;
    EnvelopePresetGenerator::PresetShape currentPresetShape = EnvelopePresetGenerator::PresetShape::Custom;
    
    std::unique_ptr<juce::ToggleButton> snapToGridButton;
    bool snapToGridEnabled = true;
}; 