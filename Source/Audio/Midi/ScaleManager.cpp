#include "ScaleManager.h"
#include "../../Audio/PluginProcessor.h"

ScaleManager::ScaleManager(PluginProcessor &p) : processor(p) {
    resetArpeggiator();

    std::vector<StructParameter<Models::MelodySettings>::FieldDescriptor> descriptors = {
            makeFieldDescriptor(Params::ID_SEMITONES_PROB, &Models::MelodySettings::semitoneProbability),
            makeFieldDescriptor(Params::ID_SEMITONES, &Models::MelodySettings::semitoneValue),
            makeFieldDescriptor(Params::ID_SEMITONES_DIRECTION, &Models::MelodySettings::semitoneDirection),
            makeFieldDescriptor(Params::ID_OCTAVES_PROB, &Models::MelodySettings::octaveProbability),
            makeFieldDescriptor(Params::ID_OCTAVES, &Models::MelodySettings::octaveValue),
            makeFieldDescriptor(Params::ID_SCALE_TYPE, &Models::MelodySettings::scaleType),
    };

    settingsBinding = std::make_unique<StructParameter<Models::MelodySettings>>(
            processor.getModulationMatrix(), descriptors);
}

void ScaleManager::resetArpeggiator() {
    currentArpStep = 0;
    arpDirectionUp = true;
}

int ScaleManager::applyScaleAndModifications(int noteNumber) {
    settings = settingsBinding->getValue();

    int finalNote = noteNumber;
    int noteRoot = noteNumber % 12;

    juce::Array<int> scale = getSelectedScale(settings.scaleType);

    if (settings.semitoneValue > 0 && settings.semitoneProbability > 0.0f) {
        if (juce::Random::getSystemRandom().nextFloat()
            < settings.semitoneProbability) {
            switch (settings.semitoneDirection) {
                case Models::DirectionType::LEFT:
                    currentArpStep--;
                    if (currentArpStep < 0)
                        currentArpStep = settings.semitoneValue;
                    break;

                case Models::DirectionType::BIDIRECTIONAL:
                    if (arpDirectionUp) {
                        currentArpStep++;
                        if (currentArpStep >= settings.semitoneValue) {
                            currentArpStep = settings.semitoneValue;
                            arpDirectionUp = false;
                        }
                    } else {
                        currentArpStep--;
                        if (currentArpStep <= 0) {
                            currentArpStep = 0;
                            arpDirectionUp = true;
                        }
                    }
                    break;

                case Models::DirectionType::RIGHT:
                    currentArpStep++;
                    if (currentArpStep > settings.semitoneValue)
                        currentArpStep = 0;
                    break;

                case Models::DirectionType::RANDOM:
                    currentArpStep = juce::Random::getSystemRandom().nextInt(
                            settings.semitoneValue + 1);
                    break;
            }

            finalNote += currentArpStep;

            finalNote = findClosestNoteInScale(finalNote, scale, noteRoot);
        } else if (!isNoteInScale(finalNote, scale)) {
            finalNote = findClosestNoteInScale(finalNote, scale, noteRoot);
        }
    } else if (!isNoteInScale(finalNote, scale)) {
        finalNote = findClosestNoteInScale(finalNote, scale, noteRoot);
    }

    if (settings.octaveValue > 0 && settings.octaveProbability > 0.0f) {
        if (juce::Random::getSystemRandom().nextFloat()
            < settings.octaveProbability) {
            int octaveAmount =
                    1 + juce::Random::getSystemRandom().nextInt(settings.octaveValue);

            if (settings.octaveBidirectional
                && juce::Random::getSystemRandom().nextBool()) {
                octaveAmount = -octaveAmount;
            }

            finalNote += octaveAmount * 12;
        }
    }

    return juce::jlimit(0, 127, finalNote);
}

bool ScaleManager::isNoteInScale(int note, const juce::Array<int> &scale) {
    int scaleDegree = (note % 12);
    return scale.contains(scaleDegree);
}

int ScaleManager::findClosestNoteInScale(int note,
                                         const juce::Array<int> &scale,
                                         int root) {
    if (isNoteInScale(note, scale))
        return note;

    int octave = note / 12;
    int closestDistance = 12;
    int closestNote = note;

    for (int scaleDegree: scale) {
        int scaleNote = (octave * 12) + scaleDegree;
        int distance = std::abs(note - scaleNote);
        if (distance < closestDistance) {
            closestDistance = distance;
            closestNote = scaleNote;
        }
    }

    return closestNote;
}

juce::Array<int> ScaleManager::getSelectedScale(Models::ScaleType scaleType) {
    switch (scaleType) {
        case Models::SCALE_MINOR:
            return Models::minorScale;
        case Models::SCALE_PENTATONIC:
            return Models::pentatonicScale;
        case Models::SCALE_MAJOR:
        default:
            return Models::majorScale;
    }
}