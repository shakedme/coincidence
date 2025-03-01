#include "MidiGeneratorProcessor.h"

//==============================================================================
MidiGeneratorProcessor::MidiGeneratorProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Initialize settings
    // Rate knobs are all at 0 by default

    // Update settings from parameters
    updateSettingsFromParameters();

    // Start timer for processing active notes
    startTimerHz(50); // 50Hz refresh rate for timer callbacks
}

MidiGeneratorProcessor::~MidiGeneratorProcessor()
{
    stopTimer();
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout MidiGeneratorProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Rate parameters
    const char* rateNames[NUM_RATE_OPTIONS] = { "1/2", "1/4", "1/8", "1/16", "1/32" };

    for (int i = 0; i < NUM_RATE_OPTIONS; ++i)
    {
        // Rate value parameter (0-100%)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "rate_" + juce::String(i) + "_value",
            "Rate " + juce::String(rateNames[i]) + " Value",
            0.0f, 100.0f, 0.0f)); // Default: 0%
    }

    // Density parameter (overall probability)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "density", "Density", 0.0f, 100.0f, 50.0f));

    // Gate parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "gate", "Gate", 0.0f, 100.0f, 50.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "gate_randomize", "Gate Randomize", 0.0f, 100.0f, 0.0f));

    // Velocity parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "velocity", "Velocity", 0.0f, 100.0f, 100.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "velocity_randomize", "Velocity Randomize", 0.0f, 100.0f, 0.0f));

    // Scale parameters
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "scale_type", "Scale Type", juce::StringArray("Major", "Minor", "Pentatonic"), 0));

    // Semitone parameters
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "semitones", "Semitones", 0, 12, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "semitones_prob", "Semitones Probability", 0.0f, 100.0f, 0.0f));

    // Octave parameters
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "octaves", "Octaves", 0, 3, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "octaves_prob", "Octaves Probability", 0.0f, 100.0f, 0.0f));

    return layout;
}

void MidiGeneratorProcessor::updateSettingsFromParameters()
{
    // Update rate settings
    for (int i = 0; i < NUM_RATE_OPTIONS; ++i)
    {
        settings.rates[i].value = *parameters.getRawParameterValue("rate_" + juce::String(i) + "_value");
    }

    // Update probability/density setting
    settings.probability = *parameters.getRawParameterValue("density");

    // Update gate settings
    settings.gate.value = *parameters.getRawParameterValue("gate");
    settings.gate.randomize = *parameters.getRawParameterValue("gate_randomize");

    // Update velocity settings
    settings.velocity.value = *parameters.getRawParameterValue("velocity");
    settings.velocity.randomize = *parameters.getRawParameterValue("velocity_randomize");

    // Update scale settings
    settings.scaleType = static_cast<ScaleType>(
        static_cast<int>(*parameters.getRawParameterValue("scale_type")));

    // Update semitone settings
    settings.semitones.value = static_cast<int>(*parameters.getRawParameterValue("semitones"));
    settings.semitones.probability = *parameters.getRawParameterValue("semitones_prob");

    // Update octave settings
    settings.octaves.value = static_cast<int>(*parameters.getRawParameterValue("octaves"));
    settings.octaves.probability = *parameters.getRawParameterValue("octaves_prob");
}

//==============================================================================
const juce::String MidiGeneratorProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MidiGeneratorProcessor::acceptsMidi() const
{
    return true;
}

bool MidiGeneratorProcessor::producesMidi() const
{
    return true;
}

bool MidiGeneratorProcessor::isMidiEffect() const
{
    return true;
}

double MidiGeneratorProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MidiGeneratorProcessor::getNumPrograms()
{
    return 1;
}

int MidiGeneratorProcessor::getCurrentProgram()
{
    return 0;
}

void MidiGeneratorProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String MidiGeneratorProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void MidiGeneratorProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void MidiGeneratorProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    this->sampleRate = sampleRate;
    samplePosition = 0;

    // Reset timing variables
    ppqPosition = 0.0;
    lastPpqPosition = 0.0;

    // Reset trigger times
    for (int i = 0; i < NUM_RATE_OPTIONS; i++) {
        lastTriggerTimes[i] = 0.0;
    }

    // Clear any active notes
    noteIsActive = false;
    isInputNoteActive = false;
    currentInputNote = -1;
    currentActiveNote = -1;
}

void MidiGeneratorProcessor::releaseResources()
{
    // Clear any active notes
    noteIsActive = false;
    isInputNoteActive = false;
    currentInputNote = -1;
    currentActiveNote = -1;
}

void MidiGeneratorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Update plugin settings from parameters
    updateSettingsFromParameters();

    // Clear audio
    buffer.clear();

    // Store the previous ppq position
    lastPpqPosition = ppqPosition;

    // Get current playhead information
    auto playHead = getPlayHead();

    if (playHead != nullptr)
    {
        juce::Optional<juce::AudioPlayHead::PositionInfo> posInfo = playHead->getPosition();

        if (posInfo.hasValue())
        {
            if (posInfo->getBpm().hasValue())
                bpm = *posInfo->getBpm();

            if (posInfo->getPpqPosition().hasValue())
                ppqPosition = *posInfo->getPpqPosition();
        }
    }

    // Process incoming MIDI messages
    juce::MidiBuffer processedMidi;

    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        const int time = metadata.samplePosition;

        // Handle note on
        if (message.isNoteOn())
        {
            currentInputNote = message.getNoteNumber();
            currentInputVelocity = message.getVelocity();
            isInputNoteActive = true;

            // If we have an active note, stop it before triggering a new one
            if (noteIsActive)
            {
                stopActiveNote(processedMidi, time);
            }
        }
        // Handle note off
        else if (message.isNoteOff() && message.getNoteNumber() == currentInputNote)
        {
            isInputNoteActive = false;

            // Send note off for the active note
            if (noteIsActive && currentActiveNote >= 0)
            {
                stopActiveNote(processedMidi, time);
            }
        }
        // Pass through all other MIDI messages
        else if (!message.isNoteOnOrOff())
        {
            processedMidi.addEvent(message, time);
        }
    }

    // Check if active note needs to be turned off within this buffer
    if (noteIsActive && isInputNoteActive)
    {
        // Calculate when the note should end (in samples relative to the start of this buffer)
        juce::int64 noteEndPosition = (noteStartTime + noteDuration) - samplePosition;

        // If the note should end during this buffer
        if (noteEndPosition >= 0 && noteEndPosition < buffer.getNumSamples())
        {
            // Send note off at the exact sample position it should end
            processedMidi.addEvent(juce::MidiMessage::noteOff(1, currentActiveNote), static_cast<int>(noteEndPosition));
            noteIsActive = false;
            currentActiveNote = -1;
        }
    }

    // Generate MIDI notes based on settings if an input note is active
    if (isInputNoteActive)
    {
        // Check each rate option for triggering a new note
        for (int rateIndex = 0; rateIndex < NUM_RATE_OPTIONS; ++rateIndex)
        {
            RateOption rate = static_cast<RateOption>(rateIndex);

            // Only consider rates with non-zero value
            if (settings.rates[rateIndex].value > 0.0f)
            {
                // Check if we should trigger a note at this rate
                if (shouldTriggerNote(rate))
                {
                    // Roll for probability based on rate intensity and density
                    float rateProb = settings.rates[rateIndex].value;
                    float overallProb = (rateProb / 100.0f) * (settings.probability / 100.0f) * 100.0f;

                    if (juce::Random::getSystemRandom().nextFloat() * 100.0f < overallProb)
                    {
                        // If there's currently a note playing, stop it
                        if (noteIsActive)
                        {
                            stopActiveNote(processedMidi, 0);
                        }

                        // Calculate randomized values for visualization
                        if (settings.gate.randomize > 0.0f) {
                            float randomRange = settings.gate.randomize / 100.0f;
                            float randomFactor = juce::Random::getSystemRandom().nextFloat() * randomRange;
                            // Store the randomized value for visualization
                            currentRandomizedGate = settings.gate.value * (1.0f + randomFactor);
                            currentRandomizedGate = juce::jlimit(0.0f, 100.0f, currentRandomizedGate);
                        } else {
                            currentRandomizedGate = settings.gate.value;
                        }

                        if (settings.velocity.randomize > 0.0f) {
                            float randomRange = settings.velocity.randomize / 100.0f;
                            float randomFactor = juce::Random::getSystemRandom().nextFloat() * randomRange;
                            // Store the randomized value for visualization
                            currentRandomizedVelocity = settings.velocity.value * (1.0f + randomFactor);
                            currentRandomizedVelocity = juce::jlimit(0.0f, 100.0f, currentRandomizedVelocity);
                        } else {
                            currentRandomizedVelocity = settings.velocity.value;
                        }

                        // Calculate note length based on rate and gate
                        int noteLengthSamples = calculateNoteLength(rate);

                        // Apply scale and modifications
                        int noteToPlay = applyScaleAndModifications(currentInputNote);

                        // Calculate velocity
                        int velocity = calculateVelocity();

                        // Add note-on message
                        processedMidi.addEvent(juce::MidiMessage::noteOn(1, noteToPlay, (juce::uint8)velocity), 0);

                        // Store the active note data
                        currentActiveNote = noteToPlay;
                        currentActiveVelocity = velocity;
                        noteStartTime = samplePosition;
                        noteDuration = noteLengthSamples;
                        noteIsActive = true;

                        // Update keyboard state
                        if (auto* editor = dynamic_cast<MidiGeneratorEditor*>(getActiveEditor())) {
                            editor->updateKeyboardState(true, currentActiveNote, currentActiveVelocity);
                        }
                    }
                }
            }
        }
    }

    // Replace original MIDI buffer with our processed one
    midiMessages.swapWith(processedMidi);

    // Update sample position
    samplePosition += buffer.getNumSamples();
}

void MidiGeneratorProcessor::stopActiveNote(juce::MidiBuffer& midiMessages, int samplePosition)
{
    if (noteIsActive && currentActiveNote >= 0)
    {
        // Send note off message - channel 1 (fixed)
        midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentActiveNote), samplePosition);

        // Update keyboard state
        if (auto* editor = dynamic_cast<MidiGeneratorEditor*>(getActiveEditor())) {
            editor->updateKeyboardState(false, currentActiveNote, 0);
        }

        noteIsActive = false;
        currentActiveNote = -1;
    }
}

//==============================================================================
bool MidiGeneratorProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* MidiGeneratorProcessor::createEditor()
{
    return new MidiGeneratorEditor(*this);
}

//==============================================================================
void MidiGeneratorProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MidiGeneratorProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
void MidiGeneratorProcessor::timerCallback()
{
    // This timer is no longer needed for note handling as we now process
    // note-offs precisely in the audio thread. This could be used for
    // other non-critical timing tasks if needed.
}

//==============================================================================
double MidiGeneratorProcessor::getNoteDurationInSamples(RateOption rate)
{
    // Calculate duration in quarter notes
    double quarterNotesPerSecond = bpm / 60.0;
    double secondsPerQuarterNote = 1.0 / quarterNotesPerSecond;

    double durationInQuarters;

    switch (rate)
    {
        case RATE_1_2:   durationInQuarters = 2.0; break;   // Half note (NEW)
        case RATE_1_4:   durationInQuarters = 1.0; break;   // Quarter note
        case RATE_1_8:   durationInQuarters = 0.5; break;   // Eighth note
        case RATE_1_16:  durationInQuarters = 0.25; break;  // Sixteenth note
        case RATE_1_32:  durationInQuarters = 0.125; break; // Thirty-second note
        default:         durationInQuarters = 1.0; break;
    }

    double durationInSeconds = secondsPerQuarterNote * durationInQuarters;
    double durationInSamples = durationInSeconds * sampleRate;

    // Return the duration, ensuring it's at least one sample
    return juce::jmax(1.0, durationInSamples);
}

bool MidiGeneratorProcessor::shouldTriggerNote(RateOption rate)
{
    // Calculate the duration in quarter notes
    double durationInQuarters;

    switch (rate)
    {
        case RATE_1_2:   durationInQuarters = 2.0; break;   // Half note (NEW)
        case RATE_1_4:   durationInQuarters = 1.0; break;
        case RATE_1_8:   durationInQuarters = 0.5; break;
        case RATE_1_16:  durationInQuarters = 0.25; break;
        case RATE_1_32:  durationInQuarters = 0.125; break;
        default:         durationInQuarters = 1.0; break;
    }

    // If PPQ position went backwards (loop point or rewind), reset the last trigger time
    if (ppqPosition < lastPpqPosition)
    {
        lastTriggerTimes[rate] = 0.0;
    }

    // Calculate how many divisions have passed since the last trigger
    double divisionsSinceLastTrigger = (ppqPosition - lastTriggerTimes[rate]) / durationInQuarters;

    // If at least one full division has passed, we should trigger again
    if (divisionsSinceLastTrigger >= 1.0)
    {
        // Update last trigger time to the closest previous division
        lastTriggerTimes[rate] = ppqPosition - std::fmod(ppqPosition, durationInQuarters);
        return true;
    }

    return false;
}

int MidiGeneratorProcessor::calculateNoteLength(RateOption rate)
{
    // Get base duration in samples for this rate
    double baseDuration = getNoteDurationInSamples(rate);

    // Apply gate percentage (0-100%)
    double gateValue = settings.gate.value / 100.0; // Convert to 0.0-1.0

    // Even at 100% gate, use exactly 95% to ensure consistent timing with a small gap
    if (gateValue > 0.95) {
        gateValue = 0.95;
    }

    // Only apply randomization if it's actually enabled
    if (settings.gate.randomize > 0.0f)
    {
        float randomRange = settings.gate.randomize / 100.0f;

        // Generate a random value between 0 and randomRange
        float randomValue = juce::Random::getSystemRandom().nextFloat() * randomRange;

        // Apply the randomization to the gate value
        gateValue += randomValue;

        // Ensure gate value is in valid range
        gateValue = juce::jlimit(0.01, 0.95, gateValue);
    }

    // Calculate final note length in samples - use a precise calculation
    int lengthInSamples = static_cast<int>(baseDuration * gateValue);

    // Minimum length safety check - at least 5ms
    int minLengthSamples = static_cast<int>(sampleRate * 0.005);
    return std::max(lengthInSamples, minLengthSamples);
}

int MidiGeneratorProcessor::calculateVelocity()
{
    // Start with base velocity value (0-127)
    double velocityValue = settings.velocity.value / 100.0 * 127.0;

    // Add randomization if needed
    if (settings.velocity.randomize > 0.0f)
    {
        float randomRange = settings.velocity.randomize / 100.0f * 127.0f;

        // Randomization (0 to +range)
        velocityValue += juce::Random::getSystemRandom().nextFloat() * randomRange;
        // Clamp between 1 and 127
        velocityValue = juce::jlimit(1.0, 127.0, velocityValue);
    }

    return static_cast<int>(velocityValue);
}

int MidiGeneratorProcessor::applyScaleAndModifications(int noteNumber)
{
    // Start with the input note
    int finalNote = noteNumber;

    // Extract the root note (0-11) and octave
    int noteRoot = noteNumber % 12;
    int octave = noteNumber / 12;

    // Get the selected scale
    juce::Array<int> scale = getSelectedScale();

    // First check if we need to apply semitone variation
    if (settings.semitones.value > 0 && settings.semitones.probability > 0.0f)
    {
        // Roll for semitone variation
        if (juce::Random::getSystemRandom().nextFloat() * 100.0f < settings.semitones.probability)
        {
            // Calculate the semitone variation (1 to max)
            int semitoneAmount = 1 + juce::Random::getSystemRandom().nextInt(settings.semitones.value);

            // If bidirectional, randomly choose up or down
            if (settings.semitones.bidirectional && juce::Random::getSystemRandom().nextBool())
            {
                semitoneAmount = -semitoneAmount;
            }

            // Apply semitone shift to the note
            finalNote += semitoneAmount;

            // Map to the closest note in scale
            finalNote = findClosestNoteInScale(finalNote, scale, noteRoot);
        }
        else if (!isNoteInScale(finalNote, scale, noteRoot))
        {
            // Even if we don't add semitones, still ensure the note is in scale
            finalNote = findClosestNoteInScale(finalNote, scale, noteRoot);
        }
    }
    else if (!isNoteInScale(finalNote, scale, noteRoot))
    {
        // Make sure the note is in the scale even if semitones are off
        finalNote = findClosestNoteInScale(finalNote, scale, noteRoot);
    }

    // Check if we need to apply octave variation (after semitones)
    if (settings.octaves.value > 0 && settings.octaves.probability > 0.0f)
    {
        // Roll for octave variation
        if (juce::Random::getSystemRandom().nextFloat() * 100.0f < settings.octaves.probability)
        {
            // Calculate the octave variation (1 to max)
            int octaveAmount = 1 + juce::Random::getSystemRandom().nextInt(settings.octaves.value);

            // If bidirectional, randomly choose up or down
            if (settings.octaves.bidirectional && juce::Random::getSystemRandom().nextBool())
            {
                octaveAmount = -octaveAmount;
            }

            // Apply octave shift
            finalNote += octaveAmount * 12;
        }
    }

    // Ensure the final note is within MIDI range (0-127)
    return juce::jlimit(0, 127, finalNote);
}

bool MidiGeneratorProcessor::isNoteInScale(int note, juce::Array<int> scale, int root)
{
    // Convert note to scale degree (0-11)
    int scaleDegree = (note % 12);

    // Check if this scale degree is in the scale
    return scale.contains(scaleDegree);
}

int MidiGeneratorProcessor::findClosestNoteInScale(int note, juce::Array<int> scale, int root)
{
    // If the note is already in the scale, return it
    if (isNoteInScale(note, scale, root))
        return note;

    // Extract the note's current octave and degree
    int octave = note / 12;
    int noteDegree = note % 12;

    // Find the closest note in the scale
    int closestDistance = 12;
    int closestNote = note;

    for (int scaleDegree : scale)
    {
        // Calculate the actual MIDI note number
        int scaleNote = (octave * 12) + scaleDegree;

        // Calculate distance
        int distance = std::abs(note - scaleNote);

        // Update closest if this is closer
        if (distance < closestDistance)
        {
            closestDistance = distance;
            closestNote = scaleNote;
        }
    }

    return closestNote;
}

juce::Array<int> MidiGeneratorProcessor::getSelectedScale()
{
    switch (settings.scaleType)
    {
        case SCALE_MINOR:
            return minorScale;
        case SCALE_PENTATONIC:
            return pentatonicScale;
        case SCALE_MAJOR:
        default:
            return majorScale;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiGeneratorProcessor();
}