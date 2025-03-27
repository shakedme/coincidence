//
// Created by Shaked Melman on 07/03/2025.
//

#ifndef JAMMER_ONSETDETECTOR_H
#define JAMMER_ONSETDETECTOR_H

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

/**
 * Class for detecting onsets in audio data based on spectral difference method
 */
class OnsetDetector {
public:
    OnsetDetector();

    ~OnsetDetector() = default;

    /**
     * Detect onsets in the given audio buffer
     * @param audioBuffer The audio buffer to analyze
     * @param sampleRate The sample rate of the audio
     * @return Vector of onset positions as normalized values (0.0-1.0)
     */
    std::vector<float> detectOnsets(const juce::AudioBuffer<float> &audioBuffer, double sampleRate);

    /**
     * Set the threshold for onset detection
     * @param threshold Value between 0.0 and 1.0
     */
    void setThreshold(float threshold) { detectionThreshold = threshold; }

    /**
     * Set the sensitivity of the onset detector
     * @param sensitivity Value between 0.0 and 1.0
     */
    void setSensitivity(float sensitivity) { detectionSensitivity = sensitivity; }

private:
    float detectionThreshold = 0.3f;

    float detectionSensitivity = 0.7f;

    std::vector<float>
    generateDetectionFunction(const juce::AudioBuffer<float> &audioBuffer, int hopSize, int frameSize);

    std::vector<int> findPeaks(const std::vector<float> &detectionFunction, float threshold);

    void normalizeBuffer(std::vector<float> &buffer);
};

#endif //JAMMER_ONSETDETECTOR_H
