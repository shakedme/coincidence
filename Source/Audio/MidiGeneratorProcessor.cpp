#include "MidiGeneratorProcessor.h"
#include "../Gui/MidiGeneratorEditor.h"

using namespace MidiGeneratorParams;

//==============================================================================
MidiGeneratorProcessor::MidiGeneratorProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Update settings from parameters
    updateSettingsFromParameters();

    // Start timer for processing active notes
    startTimerHz(50);
}

MidiGeneratorProcessor::~MidiGeneratorProcessor()
{
    stopTimer();
}

//==============================================================================
void MidiGeneratorProcessor::updateSettingsFromParameters()
{
    // Update rate settings
    for (int i = 0; i < MidiGeneratorParams::NUM_RATE_OPTIONS; ++i)
    {
        settings.rates[i].value =
            *parameters.getRawParameterValue("rate_" + juce::String(i) + "_value");
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
    settings.semitones.value =
        static_cast<int>(*parameters.getRawParameterValue("semitones"));
    settings.semitones.probability = *parameters.getRawParameterValue("semitones_prob");

    // Update octave settings
    settings.octaves.value =
        static_cast<int>(*parameters.getRawParameterValue("octaves"));
    settings.octaves.probability = *parameters.getRawParameterValue("octaves_prob");

    settings.rhythmMode = static_cast<RhythmMode>(
        static_cast<int>(*parameters.getRawParameterValue("rhythm_mode")));

    useRandomSample = *parameters.getRawParameterValue("randomize_samples") > 0.5f;
    randomizeProbability = *parameters.getRawParameterValue("randomize_probability");
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
    // If samples are loaded, we're producing audio, not MIDI
    if (sampleManager.isSampleLoaded())
        return false;

    // Otherwise we're producing MIDI
    return true;
}

bool MidiGeneratorProcessor::isMidiEffect() const
{
    // If samples are loaded, we're not just a MIDI effect
    if (sampleManager.isSampleLoaded())
        return false;

    // Otherwise behave as a MIDI effect
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
    sampleManager.prepareToPlay(sampleRate);
    samplePosition = 0;

    // Reset timing variables
    ppqPosition = 0.0;
    lastPpqPosition = 0.0;

    // Reset trigger times
    for (int i = 0; i < MidiGeneratorParams::NUM_RATE_OPTIONS; i++)
    {
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

void MidiGeneratorProcessor::updateTimingInfo()
{
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
}

// Process incoming MIDI messages
void MidiGeneratorProcessor::processIncomingMidi(const juce::MidiBuffer& midiMessages,
                                                 juce::MidiBuffer& processedMidi,
                                                 int numSamples)
{
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
}

// Check if active notes need to be turned off
void MidiGeneratorProcessor::checkActiveNotes(juce::MidiBuffer& midiMessages, int numSamples)
{
    if (noteIsActive && isInputNoteActive)
    {
        // Calculate when the note should end (in samples relative to the start of this buffer)
        juce::int64 noteEndPosition = (noteStartTime + noteDuration) - samplePosition;

        // If the note should end during this buffer
        if (noteEndPosition >= 0 && noteEndPosition < numSamples)
        {
            // Send note off at the exact sample position it should end
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentActiveNote),
                                  static_cast<int>(noteEndPosition));
            noteIsActive = false;
            currentActiveNote = -1;
            currentActiveSample = -1;
        }
    }
}

// Collect all rates that should trigger at this position
std::vector<MidiGeneratorProcessor::EligibleRate> MidiGeneratorProcessor::collectEligibleRates(float& totalWeight)
{
    std::vector<EligibleRate> eligibleRates;
    totalWeight = 0.0f;

    // Collect all rates that should trigger at this position
    for (int rateIndex = 0; rateIndex < MidiGeneratorParams::NUM_RATE_OPTIONS; ++rateIndex)
    {
        auto rate = static_cast<MidiGeneratorParams::RateOption>(rateIndex);

        // Only consider rates with non-zero value
        if (settings.rates[rateIndex].value > 0.0f)
        {
            // Check if we should trigger a note at this rate
            if (shouldTriggerNote(rate))
            {
                // Calculate weight based on rate intensity and density
                float rateWeight = settings.rates[rateIndex].value;
                float overallWeight =
                    (rateWeight / 100.0f) * (settings.probability / 100.0f) * 100.0f;

                if (overallWeight > 0.0f)
                {
                    eligibleRates.push_back({rate, overallWeight});
                    totalWeight += overallWeight;
                }
            }
        }
    }

    return eligibleRates;
}

// Select a rate from eligible rates based on weighted probability
MidiGeneratorParams::RateOption MidiGeneratorProcessor::selectRateFromEligible(
    const std::vector<EligibleRate>& eligibleRates, float totalWeight)
{
    int selectedIndex = -1;
    float randomValue = juce::Random::getSystemRandom().nextFloat();
    float cumulativeProbability = 0.0f;

    for (size_t i = 0; i < eligibleRates.size(); ++i)
    {
        // Calculate each rate's normalized probability of selection
        float rateProbability = eligibleRates[i].weight / totalWeight;
        cumulativeProbability += rateProbability;

        // If random value falls within this rate's range, select it
        if (randomValue <= cumulativeProbability)
        {
            selectedIndex = i;
            break;
        }
    }

    // If somehow we didn't select anything (floating point precision error)
    if (selectedIndex == -1 && !eligibleRates.empty())
    {
        selectedIndex = eligibleRates.size() - 1;
    }

    return eligibleRates[selectedIndex].rate;
}

// Generate new notes based on settings
void MidiGeneratorProcessor::generateNewNotes(juce::MidiBuffer& midiMessages)
{
    float totalWeight = 0.0f;
    auto eligibleRates = collectEligibleRates(totalWeight);

    // Only proceed if we have eligible rates
    if (totalWeight > 0.0f && !eligibleRates.empty())
    {
        // Determine if any note should play
        float triggerProbability = std::min(totalWeight / 100.0f, 1.0f);

        if (juce::Random::getSystemRandom().nextFloat() < triggerProbability)
        {
            // Select a rate based on weighted probability
            MidiGeneratorParams::RateOption selectedRate = selectRateFromEligible(eligibleRates, totalWeight);

            // If there's currently a note playing, stop it
            if (noteIsActive)
            {
                stopActiveNote(midiMessages, 0);
            }

            // Generate and play a new note
            playNewNote(selectedRate, midiMessages);
        }
    }
}

// Generate and play a new note with the selected rate
void MidiGeneratorProcessor::playNewNote(MidiGeneratorParams::RateOption selectedRate,
                                         juce::MidiBuffer& midiMessages)
{
    // Calculate note length based on selected rate and gate
    int noteLengthSamples = calculateNoteLength(selectedRate);

    // Apply scale and modifications
    int noteToPlay = applyScaleAndModifications(currentInputNote);

    // Calculate velocity
    int velocity = calculateVelocity();

    // Determine which sample to use (if we have samples loaded)
    int sampleIndex = -1;
    if (sampleManager.isSampleLoaded())
    {
        sampleIndex = sampleManager.getNextSampleIndex(useRandomSample, randomizeProbability);
    }

    // Add note-on message
    midiMessages.addEvent(
        juce::MidiMessage::noteOn(1, noteToPlay, (juce::uint8) velocity), 0);

    // Store the active note data
    currentActiveNote = noteToPlay;
    currentActiveVelocity = velocity;
    currentActiveSample = sampleIndex;
    noteStartTime = samplePosition;
    noteDuration = noteLengthSamples;
    noteIsActive = true;

    // Update keyboard state
    if (auto* editor = dynamic_cast<MidiGeneratorEditor*>(getActiveEditor()))
    {
        editor->updateKeyboardState(
            true, currentActiveNote, currentActiveVelocity);
    }
}

// Process audio if samples are loaded
void MidiGeneratorProcessor::processAudio(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& processedMidi,
                                          juce::MidiBuffer& midiMessages)
{
    // If we have samples loaded, process the MIDI through our sampler
    if (sampleManager.isSampleLoaded())
    {
        // Use JUCE's synthesizer to render the audio
        sampleManager.getSampler().renderNextBlock(buffer, processedMidi, 0, buffer.getNumSamples());

        // Now the buffer contains the synthesized audio
        // We clear the MIDI buffer since the sampler has processed it
        processedMidi.clear();
    }
    else
    {
        // If no samples are loaded, pass through our generated MIDI
        midiMessages.swapWith(processedMidi);
    }
}

void MidiGeneratorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Update plugin settings from parameters
    updateSettingsFromParameters();

    // Clear audio
    buffer.clear();

    // Create processed MIDI buffer
    juce::MidiBuffer processedMidi;

    // Update timing information
    updateTimingInfo();

    // Process incoming MIDI messages
    processIncomingMidi(midiMessages, processedMidi, buffer.getNumSamples());

    // Check if active notes need to be turned off
    checkActiveNotes(processedMidi, buffer.getNumSamples());

    // Generate new notes if input note is active
    if (isInputNoteActive)
    {
        generateNewNotes(processedMidi);
    }

    // Process audio through sampler if samples are loaded
    processAudio(buffer, processedMidi, midiMessages);

    // Update sample position
    samplePosition += buffer.getNumSamples();
}

float MidiGeneratorProcessor::applyRandomization(float value, float randomizeValue) const
{
    float maxValue = juce::jmin(100.0f, value + randomizeValue);
    return juce::jmap(juce::Random::getSystemRandom().nextFloat(), value, maxValue) / 100;
}

void MidiGeneratorProcessor::stopActiveNote(juce::MidiBuffer& midiMessages,
                                           int currentSamplePosition)
{
    if (noteIsActive && currentActiveNote >= 0)
    {
        // Send note off message - channel 1 (fixed)
        midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentActiveNote),
                             currentSamplePosition);

        // Update keyboard state
        if (auto* editor = dynamic_cast<MidiGeneratorEditor*>(getActiveEditor()))
        {
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

    if (xmlState != nullptr)
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
        case RATE_1_2:
            durationInQuarters = 2.0;
            break; // Half note
        case RATE_1_4:
            durationInQuarters = 1.0;
            break; // Quarter note
        case RATE_1_8:
            durationInQuarters = 0.5;
            break; // Eighth note
        case RATE_1_16:
            durationInQuarters = 0.25;
            break; // Sixteenth note
        case RATE_1_32:
            durationInQuarters = 0.125;
            break; // Thirty-second note
        default:
            durationInQuarters = 1.0;
            break;
    }

    // Apply rhythm mode modifications
    switch (settings.rhythmMode)
    {
        case RHYTHM_DOTTED:
            durationInQuarters *= 1.5; // Dotted note = 1.5x the normal duration
            break;
        case RHYTHM_TRIPLET:
            durationInQuarters *= 2.0 / 3.0; // Triplet = 2/3 the normal duration
            break;
        case RHYTHM_NORMAL:
        default:
            // No modification for normal rhythm
            break;
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
        case RATE_1_2:
            durationInQuarters = 2.0;
            break; // Half note
        case RATE_1_4:
            durationInQuarters = 1.0;
            break;
        case RATE_1_8:
            durationInQuarters = 0.5;
            break;
        case RATE_1_16:
            durationInQuarters = 0.25;
            break;
        case RATE_1_32:
            durationInQuarters = 0.125;
            break;
        default:
            durationInQuarters = 1.0;
            break;
    }

    // Apply rhythm mode modifications
    switch (settings.rhythmMode)
    {
        case RHYTHM_DOTTED:
            durationInQuarters *= 1.5; // Dotted note = 1.5x the normal duration
            break;
        case RHYTHM_TRIPLET:
            durationInQuarters *= 2.0 / 3.0; // Triplet = 2/3 the normal duration
            break;
        case RHYTHM_NORMAL:
        default:
            // No modification for normal rhythm
            break;
    }

    // If PPQ position went backwards (loop point or rewind), reset the last trigger time
    if (ppqPosition < lastPpqPosition)
    {
        lastTriggerTimes[rate] = 0.0;
    }

    // Calculate how many divisions have passed since the last trigger
    double divisionsSinceLastTrigger =
        (ppqPosition - lastTriggerTimes[rate]) / durationInQuarters;

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

    // Only apply randomization if it's actually enabled
    if (settings.gate.randomize > 0.0f)
    {
        gateValue = applyRandomization(settings.gate.value, settings.gate.randomize);
        currentRandomizedGate = static_cast<float>(gateValue) * 100;
    }

    gateValue = juce::jlimit(0.01, 0.95, gateValue);

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
        velocityValue =
            applyRandomization(settings.velocity.value, settings.velocity.randomize);
        currentRandomizedVelocity = static_cast<float>(velocityValue) * 100;
        velocityValue = velocityValue * 127.0f;
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
        if (juce::Random::getSystemRandom().nextFloat() * 100.0f
            < settings.semitones.probability)
        {
            // Calculate the semitone variation (1 to max)
            int semitoneAmount =
                1 + juce::Random::getSystemRandom().nextInt(settings.semitones.value);

            // If bidirectional, randomly choose up or down
            if (settings.semitones.bidirectional
                && juce::Random::getSystemRandom().nextBool())
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
        if (juce::Random::getSystemRandom().nextFloat() * 100.0f
            < settings.octaves.probability)
        {
            // Calculate the octave variation (1 to max)
            int octaveAmount =
                1 + juce::Random::getSystemRandom().nextInt(settings.octaves.value);

            // If bidirectional, randomly choose up or down
            if (settings.octaves.bidirectional
                && juce::Random::getSystemRandom().nextBool())
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

int MidiGeneratorProcessor::findClosestNoteInScale(int note,
                                                  juce::Array<int> scale,
                                                  int root)
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

    for (int scaleDegree: scale)
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

juce::String MidiGeneratorProcessor::getRhythmModeText(RhythmMode mode) const
{
    switch (mode)
    {
        case RHYTHM_DOTTED:
            return "D";
        case RHYTHM_TRIPLET:
            return "T";
        case RHYTHM_NORMAL:
        default:
            return "";
    }
}

// Sample Management Forwarding Methods
void MidiGeneratorProcessor::addSample(const juce::File& file)
{
    sampleManager.addSample(file);
}

void MidiGeneratorProcessor::removeSample(int index)
{
    sampleManager.removeSample(index);
}

void MidiGeneratorProcessor::clearAllSamples()
{
    sampleManager.clearAllSamples();
}

void MidiGeneratorProcessor::selectSample(int index)
{
    sampleManager.selectSample(index);
}

int MidiGeneratorProcessor::getNumSamples() const
{
    return sampleManager.getNumSamples();
}

juce::String MidiGeneratorProcessor::getSampleName(int index) const
{
    return sampleManager.getSampleName(index);
}

// AudioProcessor factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiGeneratorProcessor();
}