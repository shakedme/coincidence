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
    // Create specialized components
    audioProcessor = std::make_unique<::JammerAudioProcessor>(*this);
    noteGenerator = std::make_unique<NoteGenerator>(*this);

    // Update settings from parameters
    updateSettingsFromParameters();

    // Start timer for any background tasks
    startTimerHz(50);

    auto* fileLogger = new FileLogger();
    juce::Logger::setCurrentLogger(fileLogger);
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

    // Remove any code related to useRandomSample or randomizeProbability
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
    if (getSampleManager().isSampleLoaded())
        return false;

    // Otherwise we're producing MIDI
    return true;
}

bool PluginProcessor::isMidiEffect() const
{
    // If samples are loaded, we're not just a MIDI effect
    if (getSampleManager().isSampleLoaded())
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
    // Initialize components
    audioProcessor->prepareToPlay(sampleRate, samplesPerBlock);
    noteGenerator->prepareToPlay(sampleRate, samplesPerBlock);
}

void PluginProcessor::releaseResources()
{
    // Release components
    audioProcessor->releaseResources();
    noteGenerator->releaseResources();
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

    noteGenerator->timingManager.updateTimingInfo(getPlayHead());

    // Process incoming MIDI messages
    noteGenerator->processIncomingMidi(
        midiMessages, processedMidi, buffer.getNumSamples());

    // Check if active notes need to be turned off
    noteGenerator->checkActiveNotes(processedMidi, buffer.getNumSamples());

    // Process any pending notes scheduled from previous buffers
    noteGenerator->processPendingNotes(processedMidi, buffer.getNumSamples());

    // Generate new notes if input note is active and no note is currently playing
    if (noteGenerator->isInputNoteActive() && !noteGenerator->isNoteActive())
    {
        noteGenerator->generateNewNotes(processedMidi, settings);
    }

    // Process audio through sampler if samples are loaded
    audioProcessor->processAudio(buffer, processedMidi, midiMessages);
    noteGenerator->timingManager.updateSamplePosition(buffer.getNumSamples());
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
    // This timer is used for non-critical timing tasks if needed
}

//==============================================================================

SampleManager& PluginProcessor::getSampleManager() const
{
    return audioProcessor->getSampleManager();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}

Params::DirectionType PluginProcessor::getSampleDirectionType() const
{
    auto* param = parameters.getParameter("sample_direction");
    if (param)
    {
        auto index = static_cast<juce::AudioParameterChoice*>(param)->getIndex();
        return static_cast<Params::DirectionType>(index);
    }
    return Params::RIGHT; // Default to random
}