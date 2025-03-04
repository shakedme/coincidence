#include "ScaleManager.h"

ScaleManager::ScaleManager()
{
    // Initialize member variables
    resetArpeggiator();
}

void ScaleManager::resetArpeggiator()
{
    currentArpStep = 0;
    arpDirectionUp = true;
}

int ScaleManager::applyScaleAndModifications(int noteNumber,
                                             const Params::GeneratorSettings& settings)
{
    // Start with the input note
    int finalNote = noteNumber;

    // Extract the root note (0-11) and octave
    int noteRoot = noteNumber % 12;
    int octave = noteNumber / 12;

    // Get the selected scale
    juce::Array<int> scale = getSelectedScale(settings.scaleType);

    // First check if we need to apply semitone variation
    if (settings.semitones.value > 0 && settings.semitones.probability > 0.0f)
    {
        // Roll for semitone variation
        if (juce::Random::getSystemRandom().nextFloat() * 100.0f
            < settings.semitones.probability)
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
                    // Up
                    currentArpStep++;
                    if (currentArpStep > settings.semitones.value)
                        currentArpStep = 0;
                    break;

                case Params::DirectionType::RANDOM:
                    // Random
                    currentArpStep = juce::Random::getSystemRandom().nextInt(
                        settings.semitones.value + 1);
                    break;
            }

            // Apply step to create the arpeggiator pattern
            finalNote += currentArpStep;

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

bool ScaleManager::isNoteInScale(int note, const juce::Array<int>& scale, int root)
{
    // Convert note to scale degree (0-11)
    int scaleDegree = (note % 12);

    // Check if this scale degree is in the scale
    return scale.contains(scaleDegree);
}

int ScaleManager::findClosestNoteInScale(int note,
                                         const juce::Array<int>& scale,
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

juce::Array<int> ScaleManager::getSelectedScale(Params::ScaleType scaleType)
{
    switch (scaleType)
    {
        case Params::SCALE_MINOR:
            return Params::minorScale;
        case Params::SCALE_PENTATONIC:
            return Params::pentatonicScale;
        case Params::SCALE_MAJOR:
        default:
            return Params::majorScale;
    }
}