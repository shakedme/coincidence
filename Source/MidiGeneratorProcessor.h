#pragma once

#include "juce_audio_utils/juce_audio_utils.h"
//==============================================================================
/**
 * MidiGeneratorProcessor - Main processor class for the MIDI generator plugin
 */
class MidiGeneratorProcessor : public juce::AudioProcessor,
                               private juce::Timer
{
public:
    //==============================================================================
    MidiGeneratorProcessor();
    ~MidiGeneratorProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Timer callback
    void timerCallback() override;

    bool isNoteActive() const { return noteIsActive; }

    // Parameters
    enum RateOption {
        RATE_1_2 = 0,    // Half note
        RATE_1_4,        // Quarter note
        RATE_1_8,        // Eighth note
        RATE_1_16,       // Sixteenth note
        RATE_1_32,       // Thirty-second note
        NUM_RATE_OPTIONS
    };

    enum ScaleType {
        SCALE_MAJOR = 0,
        SCALE_MINOR,
        SCALE_PENTATONIC,
        NUM_SCALE_TYPES
    };

    struct RateSettings {
        float value = 0.0f;       // 0-100% intensity
    };

    struct GateSettings {
        float value = 50.0f;         // 0-100%
        float randomize = 0.0f;      // 0-100% (how much to randomize the gate)
    };

    struct VelocitySettings {
        float value = 100.0f;        // 0-100%
        float randomize = 0.0f;      // 0-100% (how much to randomize the velocity)
    };

    struct SemitoneSettings {
        int value = 0;               // Number of semitones
        float probability = 0.0f;    // 0-100% chance of modifying note by semitones
        bool bidirectional = false;  // Whether to allow negative semitones
    };

    struct OctaveSettings {
        int value = 0;               // Number of octaves
        float probability = 0.0f;    // 0-100% chance of modifying note by octaves
        bool bidirectional = false;  // Whether to allow negative semitones
    };

    struct GeneratorSettings {
        // Rhythm settings
        RateSettings rates[NUM_RATE_OPTIONS];
        GateSettings gate;
        VelocitySettings velocity;
        float probability = 100.0f;  // 0-100% chance of triggering a note

        // Melody settings
        ScaleType scaleType = SCALE_MAJOR;
        SemitoneSettings semitones;
        OctaveSettings octaves;
    };

    // Audio parameters tree for automation and preset support
    juce::AudioProcessorValueTreeState parameters;

    // Getter methods for the randomized values
    float getCurrentRandomizedGate() const { return currentRandomizedGate; }
    float getCurrentRandomizedVelocity() const { return currentRandomizedVelocity; }

private:
    //==============================================================================
    // Plugin state
    GeneratorSettings settings;

    // Scale patterns (semitone intervals from root)
    static inline const juce::Array<int> majorScale = { 0, 2, 4, 5, 7, 9, 11 };
    static inline const juce::Array<int> minorScale = { 0, 2, 3, 5, 7, 8, 10 };
    static inline const juce::Array<int> pentatonicScale = { 0, 2, 4, 7, 9 };

    // MIDI handling - monophonic
    int currentActiveNote = -1;
    int currentActiveVelocity = 0;
    juce::int64 noteStartTime = 0;
    juce::int64 noteDuration = 0;
    bool noteIsActive = false;

    int currentInputNote = -1;
    int currentInputVelocity = 0;
    bool isInputNoteActive = false;

    // Timing
    double sampleRate = 44100.0;
    juce::int64 samplePosition = 0;
    double bpm = 120.0;
    double ppqPosition = 0.0;
    double lastPpqPosition = 0.0;
    double lastTriggerTimes[NUM_RATE_OPTIONS] = {0.0, 0.0, 0.0, 0.0, 0.0};

    // Current randomized values for visualization
    float currentRandomizedGate = 0.0f;
    float currentRandomizedVelocity = 0.0f;

    // Helper methods
    double getNoteDurationInSamples(RateOption rate);
    bool shouldTriggerNote(RateOption rate);
    int calculateNoteLength(RateOption rate);
    int calculateVelocity();
    int applyScaleAndModifications(int noteNumber);
    bool isNoteInScale(int note, juce::Array<int> scale, int root);
    int findClosestNoteInScale(int note, juce::Array<int> scale, int root);
    juce::Array<int> getSelectedScale();
    void stopActiveNote(juce::MidiBuffer& midiMessages, int samplePosition);
    void updateSettingsFromParameters();
    float applyRandomization(float value, float randomizeValue) const;

    // Create the parameter layout
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiGeneratorProcessor)
};

//==============================================================================
/**
 * Custom look and feel for the plugin UI
 */
class MidiGeneratorLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MidiGeneratorLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g, bool isMouseOver, bool isMouseDown) override;
};

class MidiGeneratorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    MidiGeneratorEditor(MidiGeneratorProcessor&);
    ~MidiGeneratorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

    // Method to update keyboard state with played notes
    void updateKeyboardState(bool isNoteOn, int noteNumber, int velocity);

private:
    // Reference to the processor
    MidiGeneratorProcessor& processor;

    // Custom look and feel
    MidiGeneratorLookAndFeel customLookAndFeel;

    // Section labels
    std::unique_ptr<juce::Label> grooveSectionLabel;
    std::unique_ptr<juce::Label> pitchSectionLabel;
    std::unique_ptr<juce::Label> glitchSectionLabel;

    // Rhythm/Groove section components
    std::array<std::unique_ptr<juce::Slider>, MidiGeneratorProcessor::NUM_RATE_OPTIONS> rateKnobs;
    std::array<std::unique_ptr<juce::Label>, MidiGeneratorProcessor::NUM_RATE_OPTIONS> rateLabels;
    std::unique_ptr<juce::Slider> densityKnob;
    std::unique_ptr<juce::Label> densityLabel;

    std::unique_ptr<juce::Slider> gateKnob;
    std::unique_ptr<juce::Slider> gateRandomKnob;
    std::unique_ptr<juce::Label> gateLabel;
    std::unique_ptr<juce::Label> gateRandomLabel;

    std::unique_ptr<juce::Slider> velocityKnob;
    std::unique_ptr<juce::Slider> velocityRandomKnob;
    std::unique_ptr<juce::Label> velocityLabel;
    std::unique_ptr<juce::Label> velocityRandomLabel;

    // Melody/Pitch section components
    std::unique_ptr<juce::ComboBox> scaleTypeComboBox;
    std::unique_ptr<juce::Label> scaleLabel;

    std::unique_ptr<juce::Slider> shifterKnob;
    std::unique_ptr<juce::Label> shifterLabel;

    std::unique_ptr<juce::Slider> semitonesKnob;
    std::unique_ptr<juce::Slider> semitonesProbabilityKnob;
    std::unique_ptr<juce::Label> semitonesLabel;
    std::unique_ptr<juce::Label> semitonesProbabilityLabel;

    std::unique_ptr<juce::Slider> octavesKnob;
    std::unique_ptr<juce::Slider> octavesProbabilityKnob;
    std::unique_ptr<juce::Label> octavesLabel;
    std::unique_ptr<juce::Label> octavesProbabilityLabel;

    // Glitch section components (new)
    std::array<std::unique_ptr<juce::Slider>, 6> glitchKnobs;
    std::array<std::unique_ptr<juce::Label>, 6> glitchLabels;

    // Keyboard state and component
    std::unique_ptr<juce::MidiKeyboardState> keyboardState;
    std::unique_ptr<juce::MidiKeyboardComponent> keyboardComponent;

    // Attachment objects for connecting UI to parameters
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboBoxAttachments;

    // Setup methods for main sections
    void setupGrooveSection();
    void setupPitchSection();
    void setupGlitchSection();
    void setupKeyboard();

    // Setup methods for individual control groups
    void setupRateControls();
    void setupDensityControls();
    void setupGateControls();
    void setupVelocityControls();
    void setupScaleTypeControls();
    void setupShifterControls();
    void setupSemitoneControls();
    void setupOctaveControls();

    void timerCallback() override;
    void repaintRandomizationControls();

    // Utility methods
    juce::Label* createLabel(const juce::String& text, juce::Justification justification = juce::Justification::centred);
    juce::Slider* createRotarySlider(const juce::String& tooltip);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiGeneratorEditor)
};