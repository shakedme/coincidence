#include "PluginProcessor.h"
#include "../Gui/PluginEditor.h"
#include "Effects/FxEngine.h"

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
    fxEngine = std::make_unique<FxEngine>(timingManager, *this);


    // Update settings from parameters
    updateMidiSettingsFromParameters();
    updateFxSettingsFromParameters();

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

    bool pitchFollowEnabled = static_cast<bool>(*parameters.getRawParameterValue("sample_pitch_follow"));
    SamplerVoice::setPitchFollowEnabled(pitchFollowEnabled);
}

void PluginProcessor::updateFxSettingsFromParameters()
{
    // Update glitch settings from parameters
    fxSettings.stutterProbability = *parameters.getRawParameterValue("glitch_stutter");
    
    // Update reverb settings from parameters
    fxSettings.reverbMix = *parameters.getRawParameterValue("reverb_mix");
    fxSettings.reverbProbability = *parameters.getRawParameterValue("reverb_probability");
    fxSettings.reverbTime = *parameters.getRawParameterValue("reverb_time");
    fxSettings.reverbDamping = *parameters.getRawParameterValue("reverb_damping");
    fxSettings.reverbWidth = *parameters.getRawParameterValue("reverb_width");

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
    // Create a fresh XmlElement for our state (not using parameters.copyState() directly)
    auto mainXml = std::make_unique<juce::XmlElement>("JAMMER_STATE");
    
    // Add parameter state as a child
    auto paramsXml = parameters.copyState().createXml();
    mainXml->addChildElement(paramsXml.release());
    
    // Add direction information explicitly
    auto* directionXml = new juce::XmlElement("Direction");
    directionXml->setAttribute("type", static_cast<int>(getSampleDirectionType()));
    mainXml->addChildElement(directionXml);

    // Add sample information to the XML
    auto* samplesXml = new juce::XmlElement("Samples");

    // Get sample manager reference
    SampleManager& sampleManager = getSampleManager();

    // Add each loaded sample to the XML
    for (int i = 0; i < sampleManager.getNumSamples(); ++i)
    {
        auto* sampleXml = new juce::XmlElement("Sample");

        // Add sample path
        sampleXml->setAttribute("path", sampleManager.getSampleFilePath(i).getFullPathName());

        // Add sample markers if available
        if (auto* sound = sampleManager.getSampleSound(i))
        {
            sampleXml->setAttribute("startMarker", sound->getStartMarkerPosition());
            sampleXml->setAttribute("endMarker", sound->getEndMarkerPosition());
            
            // Add group index
            sampleXml->setAttribute("groupIndex", sound->getGroupIndex());
        }
        
        // Add sample probability
        sampleXml->setAttribute("probability", sampleManager.getSampleProbability(i));

        samplesXml->addChildElement(sampleXml);
    }
    
    // Add groups to the XML
    auto* groupsXml = new juce::XmlElement("Groups");
    
    // Add each group
    for (int i = 0; i < sampleManager.getNumGroups(); ++i)
    {
        if (const auto* group = sampleManager.getGroup(i))
        {
            auto* groupXml = new juce::XmlElement("Group");
            
            // Add group index
            groupXml->setAttribute("index", i);
            
            // Add group name if available
            if (group->name.isNotEmpty())
                groupXml->setAttribute("name", group->name);
            
            // Add group probability
            groupXml->setAttribute("probability", sampleManager.getGroupProbability(i));
            
            groupsXml->addChildElement(groupXml);
        }
    }

    // Add samples and groups elements to the main XML
    mainXml->addChildElement(samplesXml);
    mainXml->addChildElement(groupsXml);

    // Copy XML to binary data
    copyXmlToBinary(*mainXml, destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
    {

        // Handle both formats - either direct parameters or our custom container
        juce::XmlElement* paramsXml = nullptr;
        
        if (xmlState->hasTagName("JAMMER_STATE"))
        {
            // New format - find the parameters element (first child)
            paramsXml = xmlState->getFirstChildElement();
        }
        else if (xmlState->hasTagName(parameters.state.getType()))
        {
            // Old format - the root element is the parameters
            paramsXml = xmlState.get();
        }
        
        // Restore parameters if found
        if (paramsXml != nullptr && paramsXml->hasTagName(parameters.state.getType()))
        {
            parameters.replaceState(juce::ValueTree::fromXml(*paramsXml));
        }

        // Check for explicit direction information (in case it wasn't saved in the parameters)
        if (juce::XmlElement* directionXml = xmlState->getChildByName("Direction"))
        {
            int directionType = directionXml->getIntAttribute("type", static_cast<int>(Params::BIDIRECTIONAL));
            auto* param = parameters.getParameter("sample_direction");
            if (param)
            {
                param->setValueNotifyingHost(param->convertTo0to1(directionType));
            }
        }

        // Now look for samples
        if (juce::XmlElement* samplesXml = xmlState->getChildByName("Samples"))
        {
            // Get sample manager reference
            SampleManager& sampleManager = getSampleManager();

            // Clear existing samples
            sampleManager.clearAllSamples();

            // Load each sample
            int sampleCount = 0;
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
                                sampleCount++;

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
                                
                                // Set sample probability if it exists
                                if (sampleXml->hasAttribute("probability"))
                                {
                                    float probability = (float)sampleXml->getDoubleAttribute("probability", 1.0);
                                    sampleManager.setSampleProbability(newSampleIndex, probability);
                                }
                                
                                // Store group index for later assignment (after all groups are loaded)
                                if (sampleXml->hasAttribute("groupIndex"))
                                {
                                    int groupIndex = sampleXml->getIntAttribute("groupIndex", -1);
                                    if (groupIndex >= 0)
                                    {
                                        auto* sound = sampleManager.getSampleSound(newSampleIndex);
                                        if (sound != nullptr)
                                        {
                                            sound->setGroupIndex(groupIndex);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // After loading all samples, restore groups
            if (juce::XmlElement* groupsXml = xmlState->getChildByName("Groups"))
            {
                int groupCount = 0;
                
                // Load each group
                for (int i = 0; i < groupsXml->getNumChildElements(); ++i)
                {
                    if (auto* groupXml = groupsXml->getChildElement(i))
                    {
                        if (groupXml->hasTagName("Group"))
                        {
                            int groupIndex = groupXml->getIntAttribute("index", -1);
                            
                            if (groupIndex >= 0)
                            {
                                // Collect all samples that belong to this group
                                juce::Array<int> sampleIndices;
                                
                                for (int j = 0; j < sampleManager.getNumSamples(); ++j)
                                {
                                    if (auto* sound = sampleManager.getSampleSound(j))
                                    {
                                        if (sound->getGroupIndex() == groupIndex)
                                        {
                                            sampleIndices.add(j);
                                        }
                                    }
                                }
                                
                                // Create the group if there are samples in it
                                if (!sampleIndices.isEmpty())
                                {
                                    sampleManager.createGroup(sampleIndices);
                                    groupCount++;
                                    
                                    // Set group probability if it exists
                                    if (groupXml->hasAttribute("probability"))
                                    {
                                        float probability = (float)groupXml->getDoubleAttribute("probability", 1.0);
                                        sampleManager.setGroupProbability(groupIndex, probability);
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