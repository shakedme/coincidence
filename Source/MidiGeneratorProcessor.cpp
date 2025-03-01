#include "MidiGeneratorProcessor.h"

//==============================================================================
MidiGeneratorProcessor::MidiGeneratorProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
    , thumbnailCache(5) // cache 5 thumbnails
{
    // Initialize format manager
    formatManager.registerBasicFormats();

    // Set up synth voices
    for (int i = 0; i < 16; ++i)
        sampler.addVoice(new SamplerVoice());

    sampler.setNoteStealingEnabled(true);

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
juce::AudioProcessorValueTreeState::ParameterLayout
    MidiGeneratorProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Rate parameters
    const char* rateNames[NUM_RATE_OPTIONS] = {"1/2", "1/4", "1/8", "1/16", "1/32"};

    for (int i = 0; i < NUM_RATE_OPTIONS; ++i)
    {
        // Rate value parameter (0-100%)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "rate_" + juce::String(i) + "_value",
            "Rate " + juce::String(rateNames[i]) + " Value",
            0.0f,
            100.0f,
            0.0f)); // Default: 0%
    }

    // Density parameter (overall probability)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "density", "Density", 0.0f, 100.0f, 50.0f));

    // Gate parameters
    layout.add(
        std::make_unique<juce::AudioParameterFloat>("gate", "Gate", 0.0f, 100.0f, 50.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "gate_randomize", "Gate Randomize", 0.0f, 100.0f, 0.0f));

    // Velocity parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "velocity", "Velocity", 0.0f, 100.0f, 100.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "velocity_randomize", "Velocity Randomize", 0.0f, 100.0f, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "rhythm_mode",
        "Rhythm Mode",
        juce::StringArray("Normal", "Dotted", "Triplet"),
        RHYTHM_NORMAL));

    // Scale parameters
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "scale_type",
        "Scale Type",
        juce::StringArray("Major", "Minor", "Pentatonic"),
        0));

    // Semitone parameters
    layout.add(
        std::make_unique<juce::AudioParameterInt>("semitones", "Semitones", 0, 12, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "semitones_prob", "Semitones Probability", 0.0f, 100.0f, 0.0f));

    // Octave parameters
    layout.add(std::make_unique<juce::AudioParameterInt>("octaves", "Octaves", 0, 3, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "octaves_prob", "Octaves Probability", 0.0f, 100.0f, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "randomize_samples", "Randomize Samples", false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "randomize_probability", "Randomize Probability", 0.0f, 100.0f, 100.0f));

    return layout;
}

void MidiGeneratorProcessor::updateSettingsFromParameters()
{
    // Update rate settings
    for (int i = 0; i < NUM_RATE_OPTIONS; ++i)
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
    sampler.setCurrentPlaybackSampleRate(sampleRate);
    samplePosition = 0;

    // Reset timing variables
    ppqPosition = 0.0;
    lastPpqPosition = 0.0;

    // Reset trigger times
    for (int i = 0; i < NUM_RATE_OPTIONS; i++)
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

void MidiGeneratorProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midiMessages)
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
        juce::Optional<juce::AudioPlayHead::PositionInfo> posInfo =
            playHead->getPosition();

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

    for (const auto metadata: midiMessages)
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
            processedMidi.addEvent(juce::MidiMessage::noteOff(1, currentActiveNote),
                                   static_cast<int>(noteEndPosition));
            noteIsActive = false;
            currentActiveNote = -1;
            currentActiveSample = -1;
        }
    }

    // Generate MIDI notes based on settings if an input note is active
    if (isInputNoteActive)
    {
        // Improved weighted selection approach (Solution 3)
        struct EligibleRate
        {
            RateOption rate;
            float weight;
        };

        // Using a fixed-size array instead of a vector to avoid allocation
        std::array<EligibleRate, NUM_RATE_OPTIONS> eligibleRatesArray {};
        int eligibleRatesCount = 0;
        float totalWeight = 0.0f;

        // Collect all rates that should trigger at this position
        for (int rateIndex = 0; rateIndex < NUM_RATE_OPTIONS; ++rateIndex)
        {
            auto rate = static_cast<RateOption>(rateIndex);

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
                        eligibleRatesArray[eligibleRatesCount] = {rate, overallWeight};
                        eligibleRatesCount++;
                        totalWeight += overallWeight;
                    }
                }
            }
        }

        // Only proceed if we have eligible rates
        if (totalWeight > 0.0f && eligibleRatesCount > 0)
        {
            // Determine if any note should play
            float triggerProbability = std::min(totalWeight / 100.0f, 1.0f);

            if (juce::Random::getSystemRandom().nextFloat() < triggerProbability)
            {
                // Unbiased selection using normalized probabilities
                int selectedIndex = -1;
                float randomValue = juce::Random::getSystemRandom().nextFloat();
                float cumulativeProbability = 0.0f;

                for (int i = 0; i < eligibleRatesCount; ++i)
                {
                    // Calculate each rate's normalized probability of selection
                    float rateProbability = eligibleRatesArray[i].weight / totalWeight;
                    cumulativeProbability += rateProbability;

                    // If random value falls within this rate's range, select it
                    if (randomValue <= cumulativeProbability)
                    {
                        selectedIndex = i;
                        break;
                    }
                }

                // If somehow we didn't select anything (floating point precision error)
                if (selectedIndex == -1)
                {
                    selectedIndex = eligibleRatesCount - 1;
                }

                // Use the selected rate
                RateOption selectedRate = eligibleRatesArray[selectedIndex].rate;

                // If there's currently a note playing, stop it
                if (noteIsActive)
                {
                    stopActiveNote(processedMidi, 0);
                }

                // Calculate note length based on selected rate and gate
                int noteLengthSamples = calculateNoteLength(selectedRate);

                // Apply scale and modifications
                int noteToPlay = applyScaleAndModifications(currentInputNote);

                // Calculate velocity
                int velocity = calculateVelocity();

                // Determine which sample to use (if we have samples loaded)
                int sampleIndex = -1;
                if (sampleLoaded && !sampleList.empty())
                {
                    sampleIndex = getNextSampleIndex();
                }

                // Add note-on message
                processedMidi.addEvent(
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

                    // Also update the active sample in the editor if needed
                    if (sampleIndex >= 0)
                        editor->updateActiveSample(sampleIndex);
                }
            }
        }
    }

    // If we have samples loaded, process the MIDI through our sampler
    if (sampleLoaded && !sampleList.empty())
    {
        // Use JUCE's synthesizer to render the audio
        sampler.renderNextBlock(buffer, processedMidi, 0, buffer.getNumSamples());

        // Now the buffer contains the synthesized audio
        // We clear the MIDI buffer since the sampler has processed it
        processedMidi.clear();
    }
    else
    {
        // If no samples are loaded, pass through our generated MIDI
        midiMessages.swapWith(processedMidi);
    }

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

void MidiGeneratorProcessor::addSample(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (reader != nullptr)
    {
        // Create a range of notes to trigger this sample
        juce::BigInteger allNotes;
        allNotes.setRange(0, 128, true);

        // Create a new sample info and add to our list
        auto sampleIndex = sampleList.size();
        auto newSample = std::make_unique<SampleInfo>(
            file.getFileNameWithoutExtension(), file, sampleIndex);

        // Create the sampler sound
        auto samplerSound =
            new SamplerSound(file.getFileNameWithoutExtension(), *reader, allNotes);
        newSample->sound.reset(samplerSound);

        // Add the sound to the sampler
        sampler.addSound(samplerSound);

        // Add to our sample list
        sampleList.push_back(std::move(newSample));

        // If it's the first sample, select it
        if (sampleList.size() == 1)
            currentSelectedSample = 0;

        sampleLoaded = true;
    }
}

void MidiGeneratorProcessor::removeSample(int index)
{
    if (index < 0 || index >= static_cast<int>(sampleList.size()))
    {
        return;
    }

    // First, get a reference to the sound
    SamplerSound* soundToRemove = sampleList[index]->sound.get();

    // Clear the sound from the sampler first to avoid use-after-free
    sampler.clearSounds();

    // IMPORTANT: Release ownership of the pointer before erasing from vector
    // This prevents the unique_ptr from trying to delete the incomplete type
    sampleList[index]->sound.release();

    // Now it's safe to erase the element from the vector
    sampleList.erase(sampleList.begin() + index);

    // Manually delete the sound (if you want to)
    // delete soundToRemove;  // Uncomment if you need to delete it

    // Rebuild the sampler and renumber indices
    for (size_t i = 0; i < sampleList.size(); ++i)
    {
        sampleList[i]->index = i;
        sampler.addSound(sampleList[i]->sound.get());
    }

    // Update current selection
    if (sampleList.empty())
    {
        sampleLoaded = false;
        currentSelectedSample = -1;
    }
    else if (currentSelectedSample >= static_cast<int>(sampleList.size()))
    {
        currentSelectedSample = sampleList.size() - 1;
    }
}


void MidiGeneratorProcessor::clearAllSamples()
{
    sampler.clearSounds();
    sampleList.clear();
    sampleLoaded = false;
    currentSelectedSample = -1;
}

void MidiGeneratorProcessor::selectSample(int index)
{
    if (index >= 0 && index < sampleList.size())
    {
        currentSelectedSample = index;
    }
}

int MidiGeneratorProcessor::getNextSampleIndex()
{
    if (sampleList.empty())
        return -1;

    if (!useRandomSample || sampleList.size() == 1)
        return currentSelectedSample;

    // Check if we should randomize based on probability
    if (juce::Random::getSystemRandom().nextFloat() * 100.0f < randomizeProbability)
    {
        // Choose a random sample from all samples
        return juce::Random::getSystemRandom().nextInt(sampleList.size());
    }

    // Default to current selection
    return currentSelectedSample;
}

juce::String MidiGeneratorProcessor::getSampleName(int index) const
{
    if (index >= 0 && index < sampleList.size())
        return sampleList[index]->name;
    return "";
}

bool MidiGeneratorProcessor::producesMidi()
{
    // If samples are loaded, we're producing audio, not MIDI
    if (sampleLoaded && !sampleList.empty())
        return false;

    // Otherwise we're producing MIDI
    return true;
}

// Updated method to reflect that we're producing audio when samples are loaded
bool MidiGeneratorProcessor::isMidiEffect()
{
    // If samples are loaded, we're not just a MIDI effect
    if (sampleLoaded && !sampleList.empty())
        return false;

    // Otherwise behave as a MIDI effect
    return true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiGeneratorProcessor();
}