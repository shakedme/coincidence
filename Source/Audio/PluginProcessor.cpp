#include "PluginProcessor.h"
#include "../Gui/PluginEditor.h"
#include "Effects/FxEngine.h"

using namespace Models;

//==============================================================================
PluginProcessor::PluginProcessor()
        : AudioProcessor(BusesProperties()
                                 .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                 .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          apvts(*this,
                nullptr,
                "PARAMETERS",
                ParameterLoader::createParameterLayout()) {

    // init order matters! be aware
    modMatrix = std::make_unique<ModulationMatrix>(*this);
    timingManager = std::make_unique<TimingManager>();
    sampleManager = std::make_unique<::SampleManager>(*this);
    noteGenerator = std::make_unique<NoteGenerator>(*this);
    fxEngine = std::make_unique<FxEngine>(*this);

//    auto *fileLogger = new FileLogger();
//    juce::Logger::setCurrentLogger(fileLogger);

    forceParameterUpdates();
}

PluginProcessor::~PluginProcessor() {

}

//==============================================================================
const juce::String PluginProcessor::getName() const {
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const {
    return true;
}

bool PluginProcessor::producesMidi() const {
    // If samples are loaded, we're producing audio, not MIDI
    if (getSampleManager().isSampleLoaded())
        return false;

    // Otherwise we're producing MIDI
    return true;
}

bool PluginProcessor::isMidiEffect() const {
    // If samples are loaded, we're not just a MIDI effect
    if (getSampleManager().isSampleLoaded())
        return false;

    // Otherwise behave as a MIDI effect
    return true;
}

double PluginProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int PluginProcessor::getNumPrograms() {
    return 1;
}

int PluginProcessor::getCurrentProgram() {
    return 0;
}

void PluginProcessor::setCurrentProgram(int index) {
    juce::ignoreUnused(index);
}

const juce::String PluginProcessor::getProgramName(int index) {
    juce::ignoreUnused(index);
    return {};
}

void PluginProcessor::changeProgramName(int index, const juce::String &newName) {
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    sampleManager->prepareToPlay(sampleRate);
    noteGenerator->prepareToPlay(sampleRate, samplesPerBlock);
    fxEngine->prepareToPlay(sampleRate, samplesPerBlock);
}

void PluginProcessor::releaseResources() {
    noteGenerator->releaseResources();
    fxEngine->releaseResources();
}

void PluginProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                   juce::MidiBuffer &midiMessages) {

    modMatrix->calculateModulationValues();

    buffer.clear();

    juce::MidiBuffer processedMidi;

    timingManager->updateTimingInfo(getPlayHead());

    noteGenerator->processIncomingMidi(
            midiMessages, processedMidi, buffer.getNumSamples());

    if (sampleManager->isSampleLoaded()) {
        sampleManager->processAudio(buffer, processedMidi);
        fxEngine->processAudio(buffer, processedMidi);
        if (buffer.getNumChannels() > 0 && buffer.getNumSamples() > 0) {
            auto *editor = dynamic_cast<PluginEditor *>(getActiveEditor());
            editor->setWaveformAudioBuffer(buffer.getReadPointer(0), buffer.getNumSamples());
        }
    } else {
        midiMessages.swapWith(processedMidi);
    }

    timingManager->updateSamplePosition(buffer.getNumSamples());
}

//==============================================================================
bool PluginProcessor::hasEditor() const {
    return true;
}

juce::AudioProcessorEditor *PluginProcessor::createEditor() {
    return new PluginEditor(*this);
}

//==============================================================================
void PluginProcessor::getStateInformation(juce::MemoryBlock &destData) {
    auto mainXml = std::make_unique<juce::XmlElement>("COINCIDENCE_STATE");

    // Add parameter state as a child
    auto paramsXml = apvts.copyState().createXml();
    mainXml->addChildElement(paramsXml.release());

    // Add direction information explicitly
    auto *directionXml = new juce::XmlElement("Direction");
    directionXml->setAttribute("type", static_cast<int>(getSampleDirectionType()));
    mainXml->addChildElement(directionXml);

    // Add sample information to the XML
    auto *samplesXml = new juce::XmlElement("Samples");

    // Get sample manager reference
    SampleManager &sampleManager = getSampleManager();

    // Add each loaded sample to the XML
    for (int i = 0; i < sampleManager.getNumSamples(); ++i) {
        auto *sampleXml = new juce::XmlElement("Sample");

        // Add sample path
        sampleXml->setAttribute("path", sampleManager.getSampleFilePath(i).getFullPathName());

        // Add sample markers if available
        if (auto *sound = sampleManager.getSampleSound(i)) {
            sampleXml->setAttribute("startMarker", sound->getStartMarkerPosition());
            sampleXml->setAttribute("endMarker", sound->getEndMarkerPosition());

            // Add group index
            sampleXml->setAttribute("groupIndex", sound->getGroupIndex());
        }

        // Add sample probability
        sampleXml->setAttribute("probability", sampleManager.getSampleProbability(i));

        // Save rate toggle values
        sampleXml->setAttribute("rate_1_2_enabled", sampleManager.isSampleRateEnabled(i, Models::RATE_1_2));
        sampleXml->setAttribute("rate_1_4_enabled", sampleManager.isSampleRateEnabled(i, Models::RATE_1_4));
        sampleXml->setAttribute("rate_1_8_enabled", sampleManager.isSampleRateEnabled(i, Models::RATE_1_8));
        sampleXml->setAttribute("rate_1_16_enabled", sampleManager.isSampleRateEnabled(i, Models::RATE_1_16));

        samplesXml->addChildElement(sampleXml);
    }

    // Add groups to the XML
    auto *groupsXml = new juce::XmlElement("Groups");
    std::unordered_map<juce::String, Models::RateOption> rateMap = {
            {"rate_1_1_enabled",  Models::RATE_1_1},
            {"rate_1_2_enabled",  Models::RATE_1_2},
            {"rate_1_4_enabled",  Models::RATE_1_4},
            {"rate_1_8_enabled",  Models::RATE_1_8},
            {"rate_1_16_enabled", Models::RATE_1_16},
            {"rate_1_32_enabled", Models::RATE_1_32}
    };
    // Add each group
    for (int i = 0; i < sampleManager.getNumGroups(); ++i) {
        if (const auto *group = sampleManager.getGroup(i)) {
            auto *groupXml = new juce::XmlElement("Group");

            // Add group index
            groupXml->setAttribute("index", i);

            // Add group name if available
            if (group->name.isNotEmpty())
                groupXml->setAttribute("name", group->name);

            // Add group rate toggle values
            for (const auto &pair: rateMap) {
                groupXml->setAttribute(pair.first, sampleManager.isGroupRateEnabled(i, pair.second));
            }

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

// Force an update of all parameters by triggering the callbacks for each parameter
void PluginProcessor::forceParameterUpdates() {
    // Iterate through all parameters
    for (auto *param: getParameters()) {
        if (auto *juceParam = dynamic_cast<juce::AudioProcessorParameter *>(param)) {
            float value = juceParam->getValue();
            juceParam->sendValueChangedMessageToListeners(value);
        }
    }
}

void PluginProcessor::setStateInformation(const void *data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr) {

        // Handle both formats - either direct parameters or our custom container
        juce::XmlElement *paramsXml = nullptr;

        if (xmlState->hasTagName("COINCIDENCE_STATE")) {
            // New format - find the parameters element (first child)
            paramsXml = xmlState->getFirstChildElement();
        } else if (xmlState->hasTagName(apvts.state.getType())) {
            // Old format - the root element is the parameters
            paramsXml = xmlState.get();
        }

        // Restore parameters if found
        if (paramsXml != nullptr && paramsXml->hasTagName(apvts.state.getType())) {
            apvts.replaceState(juce::ValueTree::fromXml(*paramsXml));
        }

        // Check for explicit direction information (in case it wasn't saved in the parameters)
        if (juce::XmlElement *directionXml = xmlState->getChildByName("Direction")) {
            int directionType = directionXml->getIntAttribute("type", static_cast<int>(Models::BIDIRECTIONAL));
            auto *param = apvts.getParameter("sample_direction");
            if (param) {
                param->setValueNotifyingHost(param->convertTo0to1(directionType));
            }
        }

        // Now look for samples
        if (juce::XmlElement *samplesXml = xmlState->getChildByName("Samples")) {
            // Get sample manager reference
            SampleManager &sampleManager = getSampleManager();

            // Clear existing samples
            sampleManager.clearAllSamples();

            // Load each sample
            int sampleCount = 0;
            for (int i = 0; i < samplesXml->getNumChildElements(); ++i) {
                if (auto *sampleXml = samplesXml->getChildElement(i)) {
                    if (sampleXml->hasTagName("Sample")) {
                        juce::String path = sampleXml->getStringAttribute("path", "");
                        if (path.isNotEmpty()) {
                            juce::File sampleFile(path);
                            if (sampleFile.existsAsFile()) {
                                // Load the sample
                                sampleManager.addSample(sampleFile);
                                sampleCount++;

                                // Get the index of the just-added sample
                                int newSampleIndex = sampleManager.getNumSamples() - 1;

                                // Set marker positions if they exist
                                if (sampleXml->hasAttribute("startMarker") &&
                                    sampleXml->hasAttribute("endMarker")) {
                                    float startMarker = (float) sampleXml->getDoubleAttribute("startMarker", 0.0);
                                    float endMarker = (float) sampleXml->getDoubleAttribute("endMarker", 1.0);

                                    if (auto *sound = sampleManager.getSampleSound(newSampleIndex)) {
                                        sound->setMarkerPositions(startMarker, endMarker);
                                    }
                                }

                                // Set sample probability if it exists
                                if (sampleXml->hasAttribute("probability")) {
                                    float probability = (float) sampleXml->getDoubleAttribute("probability", 1.0);
                                    sampleManager.setSampleProbability(newSampleIndex, probability);
                                }

                                // Restore rate toggle values if they exist
                                if (sampleXml->hasAttribute("rate_1_2_enabled")) {
                                    bool enabled = sampleXml->getBoolAttribute("rate_1_2_enabled", true);
                                    sampleManager.setSampleRateEnabled(newSampleIndex, Models::RATE_1_2, enabled);
                                }

                                if (sampleXml->hasAttribute("rate_1_4_enabled")) {
                                    bool enabled = sampleXml->getBoolAttribute("rate_1_4_enabled", true);
                                    sampleManager.setSampleRateEnabled(newSampleIndex, Models::RATE_1_4, enabled);
                                }

                                if (sampleXml->hasAttribute("rate_1_8_enabled")) {
                                    bool enabled = sampleXml->getBoolAttribute("rate_1_8_enabled", true);
                                    sampleManager.setSampleRateEnabled(newSampleIndex, Models::RATE_1_8, enabled);
                                }

                                if (sampleXml->hasAttribute("rate_1_16_enabled")) {
                                    bool enabled = sampleXml->getBoolAttribute("rate_1_16_enabled", true);
                                    sampleManager.setSampleRateEnabled(newSampleIndex, Models::RATE_1_16, enabled);
                                }

                                // Store group index for later assignment (after all groups are loaded)
                                if (sampleXml->hasAttribute("groupIndex")) {
                                    int groupIndex = sampleXml->getIntAttribute("groupIndex", -1);
                                    if (groupIndex >= 0) {
                                        auto *sound = sampleManager.getSampleSound(newSampleIndex);
                                        if (sound != nullptr) {
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
            if (juce::XmlElement *groupsXml = xmlState->getChildByName("Groups")) {
                std::unordered_map<juce::String, Models::RateOption> rateMap = {
                        {"rate_1_1_enabled",  Models::RATE_1_1},
                        {"rate_1_2_enabled",  Models::RATE_1_2},
                        {"rate_1_4_enabled",  Models::RATE_1_4},
                        {"rate_1_8_enabled",  Models::RATE_1_8},
                        {"rate_1_16_enabled", Models::RATE_1_16},
                        {"rate_1_32_enabled", Models::RATE_1_32}
                };

                // Load each group
                for (int i = 0; i < groupsXml->getNumChildElements(); ++i) {
                    if (auto *groupXml = groupsXml->getChildElement(i)) {
                        if (groupXml->hasTagName("Group")) {
                            int groupIndex = groupXml->getIntAttribute("index", -1);

                            if (groupIndex >= 0) {
                                // Collect all samples that belong to this group
                                juce::Array<int> sampleIndices;

                                for (int j = 0; j < sampleManager.getNumSamples(); ++j) {
                                    if (auto *sound = sampleManager.getSampleSound(j)) {
                                        if (sound->getGroupIndex() == groupIndex) {
                                            sampleIndices.add(j);
                                        }
                                    }
                                }

                                // Create the group if there are samples in it
                                if (!sampleIndices.isEmpty()) {
                                    sampleManager.createGroup(sampleIndices);

                                    for (auto &ratePair: rateMap) {
                                        if (groupXml->hasAttribute(ratePair.first)) {
                                            bool enabled = groupXml->getBoolAttribute(ratePair.first, true);
                                            sampleManager.setGroupRateEnabled(groupIndex, ratePair.second, enabled);
                                        }
                                    }

                                    // Set group probability if it exists
                                    if (groupXml->hasAttribute("probability")) {
                                        auto probability = (float) groupXml->getDoubleAttribute("probability", 1.0);
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

    forceParameterUpdates();
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new PluginProcessor();
}

Models::DirectionType PluginProcessor::getSampleDirectionType() const {
    auto *param = apvts.getParameter("sample_direction");
    if (param) {
        auto index = dynamic_cast<juce::AudioParameterChoice *>(param)->getIndex();
        return static_cast<Models::DirectionType>(index);
    }
    return Models::RIGHT;
}
