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
    timingManager = std::make_shared<TimingManager>();
    noteGenerator = std::make_unique<NoteGenerator>(*this, timingManager);
    fxEngine = std::make_unique<FxEngine>(timingManager);


    // Update settings from parameters
    updateMidiSettingsFromParameters();
    updateFxSettingsFromParameters();

    // Start timer for any background tasks
    startTimerHz(50);

//    auto* fileLogger = new FileLogger();
//    juce::Logger::setCurrentLogger(fileLogger);
}

PluginProcessor::~PluginProcessor()
{
    stopTimer();
}

//==============================================================================
void PluginProcessor::updateMidiSettingsFromParameters()
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

    // Update octave settings
    settings.octaves.value =
        static_cast<int>(*parameters.getRawParameterValue("octaves"));
    settings.octaves.probability = *parameters.getRawParameterValue("octaves_prob");

    settings.rhythmMode = static_cast<RhythmMode>(
        static_cast<int>(*parameters.getRawParameterValue("rhythm_mode")));

    // Remove any code related to useRandomSample or randomizeProbability
}

void PluginProcessor::updateFxSettingsFromParameters()
{
    // Update glitch settings from parameters
    fxSettings.stutterProbability = *parameters.getRawParameterValue("glitch_stutter");
    fxEngine->setSettings(fxSettings);
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
    fxEngine->prepareToPlay(sampleRate, samplesPerBlock);
}

void PluginProcessor::releaseResources()
{
    // Release components
    audioProcessor->releaseResources();
    noteGenerator->releaseResources();
    fxEngine->releaseResources();
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages)
{
    // Update plugin settings from parameters
    updateMidiSettingsFromParameters();
    updateFxSettingsFromParameters();

    // Clear audio
    buffer.clear();

    // Create processed MIDI buffer
    juce::MidiBuffer processedMidi;

    timingManager->updateTimingInfo(getPlayHead());

    // Process incoming MIDI messages
    noteGenerator->processIncomingMidi(
        midiMessages, processedMidi, buffer.getNumSamples());

    // Check if active notes need to be turned off
    noteGenerator->checkActiveNotes(processedMidi, buffer.getNumSamples());

    // Process any pending notes scheduled from previous buffers
    noteGenerator->processPendingNotes(processedMidi, buffer.getNumSamples());

    // Generates midi based on settings
    noteGenerator->generateNewNotes(processedMidi, settings);

    // If samples are loaded, uses generated midi to trigger samples
    audioProcessor->processAudio(buffer, processedMidi, midiMessages);

    // Process audio effects
    fxEngine->processAudio(buffer, playHead, midiMessages);

    timingManager->updateSamplePosition(buffer.getNumSamples());
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
    // Get the parameter state
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    // Add sample information to the XML
    juce::XmlElement* samplesXml = new juce::XmlElement("Samples");

    // Get sample manager reference
    SampleManager& sampleManager = getSampleManager();

    // Add each loaded sample to the XML
    for (int i = 0; i < sampleManager.getNumSamples(); ++i)
    {
        juce::XmlElement* sampleXml = new juce::XmlElement("Sample");

        // Add sample path
        sampleXml->setAttribute("path", sampleManager.getSampleFilePath(i).getFullPathName());

        // Add sample markers if available
        if (auto* sound = sampleManager.getSampleSound(i))
        {
            sampleXml->setAttribute("startMarker", sound->getStartMarkerPosition());
            sampleXml->setAttribute("endMarker", sound->getEndMarkerPosition());
        }

        samplesXml->addChildElement(sampleXml);
    }

    // Add samples element to the main XML
    xml->addChildElement(samplesXml);

    // Copy XML to binary data
    copyXmlToBinary(*xml, destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
    {
        // First restore parameters
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));

        // Now look for samples
        if (juce::XmlElement* samplesXml = xmlState->getChildByName("Samples"))
        {
            // Get sample manager reference
            SampleManager& sampleManager = getSampleManager();

            // Clear existing samples
            sampleManager.clearAllSamples();

            // Load each sample
            for (int i = 0; i < samplesXml->getNumChildElements(); ++i)
            {
                if (auto* sampleXml = samplesXml->getChildElement(i))
                {
                    if (sampleXml->hasTagName("Sample"))
                    {
                        juce::String path = sampleXml->getStringAttribute("path", "");
                        if (path.isNotEmpty())
                        {
                            juce::File sampleFile(path);
                            if (sampleFile.existsAsFile())
                            {
                                // Load the sample
                                sampleManager.addSample(sampleFile);

                                // Get the index of the just-added sample
                                int newSampleIndex = sampleManager.getNumSamples() - 1;

                                // Set marker positions if they exist
                                if (sampleXml->hasAttribute("startMarker") &&
                                    sampleXml->hasAttribute("endMarker"))
                                {
                                    float startMarker = (float)sampleXml->getDoubleAttribute("startMarker", 0.0);
                                    float endMarker = (float)sampleXml->getDoubleAttribute("endMarker", 1.0);

                                    if (auto* sound = sampleManager.getSampleSound(newSampleIndex))
                                    {
                                        sound->setMarkerPositions(startMarker, endMarker);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
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