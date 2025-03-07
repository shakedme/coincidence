//
// Created by Shaked Melman on 07/03/2025.
//

#include "OnsetDetector.h"

OnsetDetector::OnsetDetector()
{
}

std::vector<float> OnsetDetector::detectOnsets(const juce::AudioBuffer<float>& audioBuffer, double sampleRate)
{
    // Parameters for onset detection
    int frameSize = 2048; // FFT frame size
    int hopSize = 512;    // Hop size between frames

    // Generate the detection function based on spectral difference
    std::vector<float> detectionFunction = generateDetectionFunction(audioBuffer, hopSize, frameSize);

    // Find peaks in the detection function
    std::vector<int> peakIndices = findPeaks(detectionFunction, detectionThreshold);

    // Convert peak indices to normalized positions (0.0-1.0)
    std::vector<float> onsets;
    for (auto index : peakIndices)
    {
        float normalizedPosition = static_cast<float>(index * hopSize) / audioBuffer.getNumSamples();
        onsets.push_back(normalizedPosition);
    }

    return onsets;
}

std::vector<float> OnsetDetector::generateDetectionFunction(const juce::AudioBuffer<float>& audioBuffer, int hopSize, int frameSize)
{
    int numSamples = audioBuffer.getNumSamples();
    int numFrames = (numSamples - frameSize) / hopSize + 1;
    std::vector<float> detectionFunction(numFrames, 0.0f);

    // Create FFT objects for spectral analysis
    juce::dsp::FFT fft(std::log2(frameSize));
    std::vector<float> fftBuffer(frameSize * 2, 0.0f); // Real and imaginary parts

    // Create a Hann window for the FFT
    juce::dsp::WindowingFunction<float> window(frameSize, juce::dsp::WindowingFunction<float>::hann);

    // Previous magnitude spectrum for comparison
    std::vector<float> prevMagnitudeSpectrum(frameSize / 2 + 1, 0.0f);

    // For each frame
    for (int i = 0; i < numFrames; ++i)
    {
        // Copy audio data to FFT buffer (real part only)
        for (int j = 0; j < frameSize; ++j)
        {
            int sampleIndex = i * hopSize + j;

            if (sampleIndex < numSamples)
            {
                fftBuffer[j] = audioBuffer.getSample(0, sampleIndex); // Mono analysis for simplicity
            }
            else
            {
                fftBuffer[j] = 0.0f; // Zero padding
            }
        }

        // Apply window function
        window.multiplyWithWindowingTable(fftBuffer.data(), frameSize);

        // Perform FFT
        fft.performRealOnlyForwardTransform(fftBuffer.data());

        // Calculate magnitude spectrum
        std::vector<float> magnitudeSpectrum(frameSize / 2 + 1, 0.0f);
        for (int j = 0; j < magnitudeSpectrum.size(); ++j)
        {
            float real = fftBuffer[j * 2];
            float imag = fftBuffer[j * 2 + 1];
            magnitudeSpectrum[j] = std::sqrt(real * real + imag * imag);
        }

        // Calculate spectral difference (rectified)
        float spectralDiff = 0.0f;
        for (int j = 0; j < magnitudeSpectrum.size(); ++j)
        {
            // Apply the half-wave rectifier (only count increases in energy)
            float diff = magnitudeSpectrum[j] - prevMagnitudeSpectrum[j];
            if (diff > 0.0f)
                spectralDiff += diff;
        }

        // Store in detection function
        detectionFunction[i] = spectralDiff;

        // Update previous magnitude spectrum
        prevMagnitudeSpectrum = magnitudeSpectrum;
    }

    // Normalize the detection function
    normalizeBuffer(detectionFunction);

    return detectionFunction;
}

std::vector<int> OnsetDetector::findPeaks(const std::vector<float>& detectionFunction, float threshold)
{
    std::vector<int> peaks;

    // Moving average for adaptive thresholding
    const int windowSize = 10;
    std::vector<float> movingAverage(detectionFunction.size(), 0.0f);

    // Calculate moving average
    for (int i = 0; i < detectionFunction.size(); ++i)
    {
        float sum = 0.0f;
        int count = 0;

        for (int j = std::max(0, i - windowSize / 2); j < std::min(static_cast<int>(detectionFunction.size()), i + windowSize / 2 + 1); ++j)
        {
            sum += detectionFunction[j];
            count++;
        }

        movingAverage[i] = sum / count;
    }

    // Peak detection with adaptive threshold
    for (int i = 1; i < detectionFunction.size() - 1; ++i)
    {
        // Calculate adaptive threshold
        float adaptiveThreshold = movingAverage[i] * threshold + (detectionSensitivity * 0.1f);

        // Check if it's a peak
        if (detectionFunction[i] > adaptiveThreshold &&
            detectionFunction[i] > detectionFunction[i - 1] &&
            detectionFunction[i] > detectionFunction[i + 1])
        {
            // Store peak index
            peaks.push_back(i);

            // Skip a few frames to avoid multiple detections of the same onset
            i += 3;
        }
    }

    return peaks;
}

void OnsetDetector::normalizeBuffer(std::vector<float>& buffer)
{
    float maxValue = 0.0f;

    // Find maximum value
    for (const auto& value : buffer)
    {
        maxValue = std::max(maxValue, std::abs(value));
    }

    // Normalize
    if (maxValue > 0.0f)
    {
        for (auto& value : buffer)
        {
            value /= maxValue;
        }
    }
}