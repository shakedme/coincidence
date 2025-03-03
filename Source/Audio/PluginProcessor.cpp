#include "PluginProcessor.h"
#include "../Gui/PluginEditor.h"

using namespace Params;

//==============================================================================
PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Update settings from parameters
    updateSettingsFromParameters();

    // Start timer for processing active notes
    startTimerHz(50);

    //    auto* fileLogger = new FileLogger();
    //    juce::Logger::setCurrentLogger(fileLogger);
}

PluginProcessor::~PluginProcessor()
{
    stopTimer();
}

//==============================================================================
void PluginProcessor::updateSettingsFromParameters()
{
    // Update rate settings
    for (int i = 0; i < Params::NUM_RATE_OPTIONS; ++i)
    {
        settings.rates[i].value =
            *parameters.getRawParameterValue("rate_" + juce::String(i) + "_value");
    }

    // Update probability/density setting
    settings.probability = *parameters.getRawParameterValue("density");

    // Update gate settings
    settings.gate.value = *parameters.getRawParameterValue("gate");
    settings.gate.randomize = *parameters.getRawParameterValue("gate_randomize");
    settings.gate.direction = static_cast<DirectionType>(
        static_cast<int>(*parameters.getRawParameterValue("gate_direction")));

    // Update velocity settings
    settings.velocity.value = *parameters.getRawParameterValue("velocity");
    settings.velocity.randomize = *parameters.getRawParameterValue("velocity_randomize");
    settings.velocity.direction = static_cast<DirectionType>(
        static_cast<int>(*parameters.getRawParameterValue("velocity_direction")));

    // Update scale settings
    settings.scaleType = static_cast<ScaleType>(
        static_cast<int>(*parameters.getRawParameterValue("scale_type")));

    settings.semitones.value =
        static_cast<int>(*parameters.getRawParameterValue("semitones"));
    settings.semitones.probability = *parameters.getRawParameterValue("semitones_prob");
    settings.semitones.direction = static_cast<DirectionType>(
        static_cast<int>(*parameters.getRawParameterValue("semitones_direction")));
    settings.semitones.arpeggiatorMode =
        *parameters.getRawParameterValue("arpeggiator_mode") > 0.5f;

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
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
    return true;
}

bool PluginProcessor::producesMidi() const
{
    // If samples are loaded, we're producing audio, not MIDI
    if (sampleManager.isSampleLoaded())
        return false;

    // Otherwise we're producing MIDI
    return true;
}

bool PluginProcessor::isMidiEffect() const
{
    // If samples are loaded, we're not just a MIDI effect
    if (sampleManager.isSampleLoaded())
        return false;

    // Otherwise behave as a MIDI effect
    return true;
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String PluginProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void PluginProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    this->sampleRate = sampleRate;
    sampleManager.prepareToPlay(sampleRate);
    samplePosition = 0;

    // Reset timing variables
    ppqPosition = 0.0;
    lastPpqPosition = 0.0;

    // Reset trigger times
    for (int i = 0; i < Params::NUM_RATE_OPTIONS; i++)
    {
        lastTriggerTimes[i] = 0.0;
    }

    // Clear any active notes
    noteIsActive = false;
    isInputNoteActive = false;
    currentInputNote = -1;
    currentActiveNote = -1;

    // Clear any pending notes
    pendingNotes.clear();
}

void PluginProcessor::releaseResources()
{
    // Clear any active notes
    noteIsActive = false;
    isInputNoteActive = false;
    currentInputNote = -1;
    currentActiveNote = -1;
}

void PluginProcessor::updateTimingInfo()
{
    // Store the previous ppq position
    lastPpqPosition = ppqPosition;
    lastContinuousPpqPosition = ppqPosition; // Save before updates

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
            {
                ppqPosition = *posInfo->getPpqPosition();

                // Detect a loop - PPQ position has jumped backward significantly
                // Small jumps backward (less than a quarter note) could be jitter, ignore those
                if (ppqPosition < lastContinuousPpqPosition - 0.25)
                {
                    loopJustDetected = true;

                    // Reset timing state for all rates
                    for (int i = 0; i < Params::NUM_RATE_OPTIONS; i++)
                    {
                        lastTriggerTimes[i] = 0.0;
                    }
                }
                else
                {
                    loopJustDetected = false;
                }
            }
        }
    }
}

// Process incoming MIDI messages
void PluginProcessor::processIncomingMidi(const juce::MidiBuffer& midiMessages,
                                          juce::MidiBuffer& processedMidi,
                                          int numSamples)
{
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
void PluginProcessor::checkActiveNotes(juce::MidiBuffer& midiMessages, int numSamples)
{
    if (noteIsActive && isInputNoteActive)
    {
        // Calculate when the note should end (in samples relative to the start of this buffer)
        juce::int64 noteEndPosition =
            (noteStartPosition + noteDurationInSamples) - samplePosition;

        // If the note should end during this buffer
        if (noteEndPosition >= 0 && noteEndPosition < numSamples)
        {
            // Send note off at the exact sample position it should end
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentActiveNote),
                                  static_cast<int>(noteEndPosition));
            noteIsActive = false;
            currentActiveNote = -1;
            currentActiveSampleIdx = -1;
        }
    }
}

// Collect all rates that should trigger at this position
std::vector<PluginProcessor::EligibleRate>
    PluginProcessor::collectEligibleRates(float& totalWeight)
{
    std::vector<EligibleRate> eligibleRates;
    totalWeight = 0.0f;

    // Collect all rates that should trigger at this position
    for (int rateIndex = 0; rateIndex < Params::NUM_RATE_OPTIONS; ++rateIndex)
    {
        auto rate = static_cast<Params::RateOption>(rateIndex);

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
Params::RateOption PluginProcessor::selectRateFromEligible(
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
void PluginProcessor::generateNewNotes(juce::MidiBuffer& midiMessages)
{
    float totalWeight = 0.0f;
    auto eligibleRates = collectEligibleRates(totalWeight);

    // Only proceed if  we have eligible rates
    if (totalWeight > 0.0f && !eligibleRates.empty())
    {
        // Determine if any note should play
        float triggerProbability = std::min(totalWeight / 100.0f, 1.0f);
        bool shouldPlayNote =
            juce::Random::getSystemRandom().nextFloat() < triggerProbability
            || settings.probability == 100.0f;

        if (shouldPlayNote)
        {
            // Select a rate based on weighted probability
            Params::RateOption selectedRate =
                selectRateFromEligible(eligibleRates, totalWeight);

            // Generate and play a new note
            playNewNote(selectedRate, midiMessages);
        }
    }
}

void PluginProcessor::playNewNote(Params::RateOption selectedRate,
                                  juce::MidiBuffer& midiMessages)
{
    // Calculate the duration in quarter notes for this rate
    double durationInQuarters;
    switch (selectedRate)
    {
        case RATE_1_2:
            durationInQuarters = 2.0;
            break;
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
            durationInQuarters *= 1.5;
            break;
        case RHYTHM_TRIPLET:
            durationInQuarters *= 2.0 / 3.0;
            break;
        default:
            break;
    }

    // Calculate next expected grid position
    double nextExpectedGridPoint;

    // Special case for loop points or first trigger
    if (loopJustDetected || lastTriggerTimes[selectedRate] <= 0.0)
    {
        // At loop points, align with the closest grid
        double gridStartPpq =
            std::floor(ppqPosition / durationInQuarters) * durationInQuarters;

        // If we're very close to a grid point, use that
        double ppqSinceGrid = ppqPosition - gridStartPpq;
        double triggerWindowInPPQ = 0.05 * std::max(1.0, bpm / 120.0);

        if (ppqSinceGrid < triggerWindowInPPQ)
        {
            nextExpectedGridPoint = gridStartPpq;
        }
        else
        {
            // Otherwise use the next grid point
            nextExpectedGridPoint = gridStartPpq + durationInQuarters;
        }
    }
    else
    {
        // Normal case - calculate next grid from last trigger
        nextExpectedGridPoint = lastTriggerTimes[selectedRate] + durationInQuarters;

        // Safety check - if next grid is too far in future, reset
        if (nextExpectedGridPoint > ppqPosition + 4.0)
        {
            double gridStartPpq =
                std::floor(ppqPosition / durationInQuarters) * durationInQuarters;
            nextExpectedGridPoint = gridStartPpq + durationInQuarters;
        }
    }

    // Calculate precise sample position for this grid point
    double samplesPerQuarterNote = (60.0 / bpm) * sampleRate;
    double ppqOffsetFromCurrent = nextExpectedGridPoint - ppqPosition;

    // Convert to sample offset - this ensures grid alignment
    int sampleOffset = static_cast<int>(ppqOffsetFromCurrent * samplesPerQuarterNote);

    // If we somehow missed the grid point, play ASAP
    if (sampleOffset < 0)
    {
        sampleOffset = 0;
    }

    juce::int64 absoluteNotePosition = samplePosition + sampleOffset;

    // Calculate note properties
    int noteLengthSamples = calculateNoteLength(selectedRate);
    const int minimumNoteDuration = static_cast<int>(sampleRate * 0.01);
    noteLengthSamples = std::max(noteLengthSamples, minimumNoteDuration);
    int noteToPlay = applyScaleAndModifications(currentInputNote);
    int velocity = calculateVelocity();

    // Determine which sample to use
    int sampleIndex = -1;
    if (sampleManager.isSampleLoaded())
    {
        sampleIndex =
            sampleManager.getNextSampleIndex(useRandomSample, randomizeProbability);
    }

    int bufferSize = getBlockSize();

    if (sampleOffset < bufferSize)
    {
        // Immediate playback - note falls within current buffer
        midiMessages.addEvent(
            juce::MidiMessage::noteOn(1, noteToPlay, (juce::uint8) velocity),
            sampleOffset);

        // Store the active note data
        currentActiveNote = noteToPlay;
        currentActiveVelocity = velocity;
        currentActiveSampleIdx = sampleIndex;
        noteStartPosition = absoluteNotePosition;
        noteDurationInSamples = noteLengthSamples;
        noteIsActive = true;

        // Update keyboard state
        if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor()))
        {
            editor->updateKeyboardState(true, currentActiveNote, currentActiveVelocity);
        }
    }
    else
    {
        // Schedule for future buffer
        PendingNote pendingNote;
        pendingNote.noteNumber = noteToPlay;
        pendingNote.velocity = velocity;
        pendingNote.startSamplePosition = absoluteNotePosition;
        pendingNote.durationInSamples = noteLengthSamples;
        pendingNote.sampleIndex = sampleIndex;

        // Add to pending notes queue
        pendingNotes.push_back(pendingNote);
    }

    // Update lastTriggerTimes to exactly the grid point we just played
    // This ensures the next note will be spaced exactly one grid interval away
    lastTriggerTimes[selectedRate] = nextExpectedGridPoint;

    // If we were in a loop, we're now past that state
    loopJustDetected = false;
}

void PluginProcessor::processPendingNotes(juce::MidiBuffer& midiMessages, int numSamples)
{
    if (pendingNotes.empty())
        return;

    // Process the pending notes that fall within the current buffer
    auto it = pendingNotes.begin();
    while (it != pendingNotes.end())
    {
        // Calculate local buffer position
        juce::int64 localPosition = it->startSamplePosition - samplePosition;

        // If the note start position is in this buffer
        if (localPosition >= 0 && localPosition < numSamples)
        {
            // Add the note at the calculated position
            midiMessages.addEvent(
                juce::MidiMessage::noteOn(1, it->noteNumber, (juce::uint8) it->velocity),
                static_cast<int>(localPosition));

            // Update the active note info
            currentActiveNote = it->noteNumber;
            currentActiveVelocity = it->velocity;
            currentActiveSampleIdx = it->sampleIndex;
            noteStartPosition = it->startSamplePosition;
            noteDurationInSamples = it->durationInSamples;
            noteIsActive = true;

            // Update keyboard state
            if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor()))
            {
                editor->updateKeyboardState(
                    true, currentActiveNote, currentActiveVelocity);
            }

            // Remove the played note from pending list
            it = pendingNotes.erase(it);
        }
        // If the note start position is before this buffer, it's too late to play it
        else if (localPosition < 0)
        {
            it = pendingNotes.erase(it);
        }
        else
        {
            // Note is still in the future
            ++it;
        }
    }
}

// Process audio if samples are loaded
void PluginProcessor::processAudio(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& processedMidi,
                                   juce::MidiBuffer& midiMessages)
{
    // If we have samples loaded, process the MIDI through our sampler
    if (sampleManager.isSampleLoaded())
    {
        // Use JUCE's synthesizer to render the audio
        sampleManager.getSampler().renderNextBlock(
            buffer, processedMidi, 0, buffer.getNumSamples());

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

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages)
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

    // Process any pending notes scheduled from previous buffers
    processPendingNotes(processedMidi, buffer.getNumSamples());

    // Generate new notes if input note is active and no note is currently playing
    if (isInputNoteActive && !noteIsActive)
    {
        generateNewNotes(processedMidi);
    }

    // Process audio through sampler if samples are loaded
    processAudio(buffer, processedMidi, midiMessages);

    // Update sample position
    samplePosition += buffer.getNumSamples();
}

float PluginProcessor::applyRandomization(float value,
                                          float randomizeValue,
                                          Params::DirectionType direction) const
{
    float maxValue = juce::jmin(100.0f, value + randomizeValue);
    float minValue = juce::jmax(0.0f, value - randomizeValue);
    float rightValue =
        juce::jmap(juce::Random::getSystemRandom().nextFloat(), value, maxValue) / 100;
    float leftValue =
        juce::jmap(juce::Random::getSystemRandom().nextFloat(), minValue, value) / 100;

    if (direction == Params::DirectionType::RIGHT)
    {
        return rightValue;
    }
    else if (direction == Params::DirectionType::LEFT)
    {
        return leftValue;
    }
    else
    {
        return juce::Random::getSystemRandom().nextFloat() > 0.5 ? rightValue : leftValue;
    }
}

void PluginProcessor::stopActiveNote(juce::MidiBuffer& midiMessages,
                                     int currentSamplePosition)
{
    if (noteIsActive && currentActiveNote >= 0)
    {
        // Send note off message - channel 1 (fixed)
        midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentActiveNote),
                              currentSamplePosition);

        // Update keyboard state
        if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor()))
        {
            editor->updateKeyboardState(false, currentActiveNote, 0);
        }

        noteIsActive = false;
        currentActiveNote = -1;
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}

//==============================================================================
void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
void PluginProcessor::timerCallback()
{
    // This timer is no longer needed for note handling as we now process
    // note-offs precisely in the audio thread. This could be used for
    // other non-critical timing tasks if needed.
}

//==============================================================================
double PluginProcessor::getNoteDurationInSamples(RateOption rate)
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

bool PluginProcessor::shouldTriggerNote(RateOption rate)
{
    // Calculate the duration in quarter notes
    double durationInQuarters;
    switch (rate)
    {
        case RATE_1_2:
            durationInQuarters = 2.0;
            break;
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
            durationInQuarters *= 1.5;
            break;
        case RHYTHM_TRIPLET:
            durationInQuarters *= 2.0 / 3.0;
            break;
        default:
            break;
    }

    // If we just detected a loop, check if we need to trigger based on current position
    if (loopJustDetected || lastTriggerTimes[rate] <= 0.0)
    {
        // Find the closest grid point at or before current position
        double gridStartPpq =
            std::floor(ppqPosition / durationInQuarters) * durationInQuarters;

        // Calculate a reasonable window for triggering (adaptive to tempo)
        double triggerWindowInPPQ = 0.05 * std::max(1.0, bpm / 120.0);

        // If we're close to a grid point at the start of the loop, we should trigger
        double ppqSinceGrid = ppqPosition - gridStartPpq;

        // Trigger if we're very close to a grid point
        if (ppqSinceGrid < triggerWindowInPPQ)
        {
            return true;
        }

        // Trigger if the next grid point falls within this buffer
        double nextGridPpq = gridStartPpq + durationInQuarters;
        double ppqSpanOfCurrentBuffer = (getBlockSize() / sampleRate) * (bpm / 60.0);
        double ppqUntilNextGrid = nextGridPpq - ppqPosition;

        if (ppqUntilNextGrid >= 0 && ppqUntilNextGrid <= ppqSpanOfCurrentBuffer)
        {
            return true;
        }

        return false;
    }

    // Normal case - not at a loop point
    // Find the next grid point that should trigger a note
    double nextExpectedGridPoint;

    // If we have a last trigger time, base next grid on that for consistency
    if (lastTriggerTimes[rate] > 0.0)
    {
        // Calculate the next grid from the last played grid
        nextExpectedGridPoint = lastTriggerTimes[rate] + durationInQuarters;

        // If the next grid is way in the future (more than 4 beats ahead),
        // we might have missed some cycles due to state issues - reset
        if (nextExpectedGridPoint > ppqPosition + 4.0)
        {
            double gridStartPpq =
                std::floor(ppqPosition / durationInQuarters) * durationInQuarters;
            nextExpectedGridPoint = gridStartPpq + durationInQuarters;
        }
    }
    else
    {
        // No previous trigger - find closest grid point
        double gridStartPpq =
            std::floor(ppqPosition / durationInQuarters) * durationInQuarters;
        nextExpectedGridPoint = gridStartPpq + durationInQuarters;
    }

    // Calculate a narrow window for triggering
    // Adaptive window size based on BPM for better performance at higher tempos
    double triggerWindowInPPQ = 0.01 * std::max(1.0, bpm / 120.0);

    // Check if we're within the window to trigger
    double ppqUntilNextGrid = nextExpectedGridPoint - ppqPosition;

    // Detect if we need to trigger now
    bool shouldTrigger = false;

    // Check if grid point is within this buffer's time span
    double ppqSpanOfCurrentBuffer = (getBlockSize() / sampleRate) * (bpm / 60.0);

    // Grid point is coming up in this buffer
    if (ppqUntilNextGrid >= 0 && ppqUntilNextGrid <= ppqSpanOfCurrentBuffer)
    {
        shouldTrigger = true;
    }
    // We already passed the next grid point (timing issue)
    else if (ppqUntilNextGrid < 0 && ppqUntilNextGrid > -triggerWindowInPPQ)
    {
        shouldTrigger = true;
    }

    return shouldTrigger;
}

int PluginProcessor::calculateNoteLength(RateOption rate)
{
    // Get base duration in samples for this rate
    double baseDuration = getNoteDurationInSamples(rate);

    // Apply gate percentage (0-100%)
    double gateValue = settings.gate.value / 100.0; // Convert to 0.0-1.0

    // Only apply randomization if it's actually enabled
    if (settings.gate.randomize > 0.0f)
    {
        gateValue = applyRandomization(
            settings.gate.value, settings.gate.randomize, settings.gate.direction);
        currentRandomizedGate = static_cast<float>(gateValue) * 100;
    }

    gateValue = juce::jlimit(0.01, 0.98, gateValue);

    // Calculate final note length in samples - use a precise calculation
    int lengthInSamples = static_cast<int>(baseDuration * gateValue);

    // Minimum length safety check - at least 5ms
    int minLengthSamples = static_cast<int>(sampleRate * 0.005);
    return std::max(lengthInSamples, minLengthSamples);
}

int PluginProcessor::calculateVelocity()
{
    // Start with base velocity value (0-127)
    double velocityValue = settings.velocity.value / 100.0 * 127.0;

    // Add randomization if needed
    if (settings.velocity.randomize > 0.0f)
    {
        velocityValue = applyRandomization(settings.velocity.value,
                                           settings.velocity.randomize,
                                           settings.velocity.direction);
        currentRandomizedVelocity = static_cast<float>(velocityValue) * 100;
        velocityValue = velocityValue * 127.0f;
        velocityValue = juce::jlimit(1.0, 127.0, velocityValue);
    }

    return static_cast<int>(velocityValue);
}

int PluginProcessor::applyScaleAndModifications(int noteNumber)
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
            if (settings.semitones.arpeggiatorMode)
            {
                // Arpeggiator mode - sequential stepping
                switch (settings.semitones.direction)
                {
                    case Params::DirectionType::LEFT:
                        // Down
                        currentArpStep--;
                        if (currentArpStep < 0)
                            currentArpStep = settings.semitones.value;
                        break;

                    case Params::DirectionType::BIDIRECTIONAL:
                        // Bidirectional (up then down)
                        if (arpDirectionUp)
                        {
                            currentArpStep++;
                            if (currentArpStep >= settings.semitones.value)
                            {
                                currentArpStep = settings.semitones.value;
                                arpDirectionUp = false;
                            }
                        }
                        else
                        {
                            currentArpStep--;
                            if (currentArpStep <= 0)
                            {
                                currentArpStep = 0;
                                arpDirectionUp = true;
                            }
                        }
                        break;

                    case Params::DirectionType::RIGHT:
                    default:
                        // Up
                        currentArpStep++;
                        if (currentArpStep > settings.semitones.value)
                            currentArpStep = 0;
                        break;
                }

                // Apply step to create the arpeggiator pattern
                finalNote += currentArpStep;

                // Map to the closest note in scale
                finalNote = findClosestNoteInScale(finalNote, scale, noteRoot);
            }
            else
            {
                // Original random mode (unchanged)
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

bool PluginProcessor::isNoteInScale(int note, juce::Array<int> scale, int root)
{
    // Convert note to scale degree (0-11)
    int scaleDegree = (note % 12);

    // Check if this scale degree is in the scale
    return scale.contains(scaleDegree);
}

int PluginProcessor::findClosestNoteInScale(int note, juce::Array<int> scale, int root)
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

juce::Array<int> PluginProcessor::getSelectedScale()
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

juce::String PluginProcessor::getRhythmModeText(RhythmMode mode) const
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
void PluginProcessor::addSample(const juce::File& file)
{
    sampleManager.addSample(file);
}

void PluginProcessor::removeSample(int index)
{
    sampleManager.removeSample(index);
}

void PluginProcessor::clearAllSamples()
{
    sampleManager.clearAllSamples();
}

void PluginProcessor::selectSample(int index)
{
    sampleManager.selectSample(index);
}

int PluginProcessor::getNumSamples() const
{
    return sampleManager.getNumSamples();
}

juce::String PluginProcessor::getSampleName(int index) const
{
    return sampleManager.getSampleName(index);
}

// AudioProcessor factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}