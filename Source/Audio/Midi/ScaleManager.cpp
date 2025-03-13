#include "ScaleManager.h"

using namespace AppState;

ScaleManager::ScaleManager() {
    // Initialize member variables
    resetArpeggiator();

    paramBinding = StateManager::getInstance().createParameterBinding<Config::MelodySettings>(settings);
    paramBinding->registerParameters(createMelodyParameters());
}

void ScaleManager::resetArpeggiator() {
    currentArpStep = 0;
    arpDirectionUp = true;
}

int ScaleManager::applyScaleAndModifications(int noteNumber) {
    // Start with the input note
    int finalNote = noteNumber;

    // Extract the root note (0-11) and octave
    int noteRoot = noteNumber % 12;

    // Get the selected scale
    juce::Array<int> scale = getSelectedScale(settings.scaleType);

    // First check if we need to apply semitone variation
    if (settings.semitoneValue > 0 && settings.semitoneProbability > 0.0f) {
        // Roll for semitone variation
        if (juce::Random::getSystemRandom().nextFloat()
            < settings.semitoneProbability) {
            // Arpeggiator mode - sequential stepping
            switch (settings.semitoneDirection) {
                case Config::DirectionType::LEFT:
                    // Down
                    currentArpStep--;
                    if (currentArpStep < 0)
                        currentArpStep = settings.semitoneValue;
                    break;

                case Config::DirectionType::BIDIRECTIONAL:
                    // Bidirectional (up then down)
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

                case Config::DirectionType::RIGHT:
                    // Up
                    currentArpStep++;
                    if (currentArpStep > settings.semitoneValue)
                        currentArpStep = 0;
                    break;

                case Config::DirectionType::RANDOM:
                    // Random
                    currentArpStep = juce::Random::getSystemRandom().nextInt(
                            settings.semitoneValue + 1);
                    break;
            }

            // Apply step to create the arpeggiator pattern
            finalNote += currentArpStep;

            // Map to the closest note in scale
            finalNote = findClosestNoteInScale(finalNote, scale, noteRoot);
        } else if (!isNoteInScale(finalNote, scale)) {
            // Even if we don't add semitones, still ensure the note is in scale
            finalNote = findClosestNoteInScale(finalNote, scale, noteRoot);
        }
    } else if (!isNoteInScale(finalNote, scale)) {
        // Make sure the note is in the scale even if semitones are off
        finalNote = findClosestNoteInScale(finalNote, scale, noteRoot);
    }

    // Check if we need to apply octave variation (after semitones)
    if (settings.octaveValue > 0 && settings.octaveProbability > 0.0f) {
        // Roll for octave variation
        if (juce::Random::getSystemRandom().nextFloat()
            < settings.octaveProbability) {
            // Calculate the octave variation (1 to max)
            int octaveAmount =
                    1 + juce::Random::getSystemRandom().nextInt(settings.octaveValue);

            // If bidirectional, randomly choose up or down
            if (settings.octaveBidirectional
                && juce::Random::getSystemRandom().nextBool()) {
                octaveAmount = -octaveAmount;
            }

            // Apply octave shift
            finalNote += octaveAmount * 12;
        }
    }

    // Ensure the final note is within MIDI range (0-127)
    return juce::jlimit(0, 127, finalNote);
}

bool ScaleManager::isNoteInScale(int note, const juce::Array<int> &scale) {
    // Convert note to scale degree (0-11)
    int scaleDegree = (note % 12);

    // Check if this scale degree is in the scale
    return scale.contains(scaleDegree);
}

int ScaleManager::findClosestNoteInScale(int note,
                                         const juce::Array<int> &scale,
                                         int root) {
    // If the note is already in the scale, return it
    if (isNoteInScale(note, scale))
        return note;

    // Extract the note's current octave and degree
    int octave = note / 12;

    // Find the closest note in the scale
    int closestDistance = 12;
    int closestNote = note;

    for (int scaleDegree: scale) {
        // Calculate the actual MIDI note number
        int scaleNote = (octave * 12) + scaleDegree;

        // Calculate distance
        int distance = std::abs(note - scaleNote);

        // Update closest if this is closer
        if (distance < closestDistance) {
            closestDistance = distance;
            closestNote = scaleNote;
        }
    }

    return closestNote;
}

juce::Array<int> ScaleManager::getSelectedScale(Config::ScaleType scaleType) {
    switch (scaleType) {
        case Config::SCALE_MINOR:
            return Config::minorScale;
        case Config::SCALE_PENTATONIC:
            return Config::pentatonicScale;
        case Config::SCALE_MAJOR:
        default:
            return Config::majorScale;
    }
}