#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../../Audio/Sampler/SampleManager.h"
#include <juce_dsp/juce_dsp.h>

class SampleDetailComponent : public juce::Component,
                              private juce::ChangeListener
{
public:
    SampleDetailComponent(SampleManager& manager)
        : sampleManager(manager)
    {
        // Initialize the thumbnail cache and source
        thumbnailCache = std::make_unique<juce::AudioThumbnailCache>(5);
        thumbnail = std::make_unique<juce::AudioThumbnail>(512, formatManager, *thumbnailCache);

        formatManager.registerBasicFormats();

        startMarkerPosition = 0.0;
        endMarkerPosition = 1.0;

        // Start listening for changes to the thumbnail
        thumbnail->addChangeListener(this);

        // Create back arrow path
        backArrowPath.startNewSubPath(10.0f, 10.0f);
        backArrowPath.lineTo(5.0f, 15.0f);
        backArrowPath.lineTo(10.0f, 20.0f);
        backArrowPath.closeSubPath();

        // Define clickable area for back arrow
        backArrowBounds = juce::Rectangle<int>(0, 0, 30, 20);
        
        // Add detect onsets button
        detectOnsetsButton = std::make_unique<juce::TextButton>("Detect Onsets");
        detectOnsetsButton->onClick = [this]() { detectBeatOnsets(); };
        addAndMakeVisible(detectOnsetsButton.get());
    }

    ~SampleDetailComponent() override
    {
        thumbnail->removeChangeListener(this);
    }

    int getSampleIndex(){
        return currentSampleIndex;
    }

    void setSampleIndex(int index)
    {
        if (currentSampleIndex != index)
        {
            currentSampleIndex = index;

            if (currentSampleIndex >= 0 && currentSampleIndex <  sampleManager.getNumSamples())
            {
                // Get the file for this sample
                juce::String name = sampleManager.getSampleName(currentSampleIndex);
                sampleName = name;

                // Get the sample sound
                if (auto* sound = sampleManager.getSampleSound(currentSampleIndex))
                {
                    startMarkerPosition = sound->getStartMarkerPosition();
                    endMarkerPosition = sound->getEndMarkerPosition();

                    // Clear previous thumbnail
                    thumbnail->clear();

                    // Create a reader for the sample data
                    auto& audioData = *sound->getAudioData();
                    double sampleRate = sound->getSourceSampleRate();

                    // Set the sample data to the thumbnail
                    thumbnail->reset(audioData.getNumChannels(), sampleRate, audioData.getNumSamples());

                    // Add the sample data to the thumbnail
                    thumbnail->addBlock(0, audioData, 0, audioData.getNumSamples());

                    repaint();
                }
            }
        }
    }

    void paint(juce::Graphics& g) override
    {
        // Fill background
        g.fillAll(juce::Colour(0xff2a2a2a));

        // Draw back arrow
        g.setColour(juce::Colours::white);
        g.strokePath(backArrowPath, juce::PathStrokeType(2.0f));

        // Draw sample name
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(16.0f)));
        g.drawText(sampleName, getLocalBounds().removeFromTop(30), juce::Justification::centred, true);

        auto bounds = getLocalBounds().reduced(10).withTrimmedTop(30);

        // Draw waveform background
        g.setColour(juce::Colour(0xff3a3a3a));
        g.fillRect(bounds);

        // Draw waveform if available
        if (thumbnail->getTotalLength() > 0.0)
        {
            g.setColour(juce::Colour(0xffbf52d9));
            thumbnail->drawChannels(g, bounds, 0.0, thumbnail->getTotalLength(), 1.0f);

            // Calculate marker positions in pixels
            float startPixel = bounds.getX() + bounds.getWidth() * startMarkerPosition;
            float endPixel = bounds.getX() + bounds.getWidth() * endMarkerPosition;

            // Draw markers
            g.setColour(juce::Colours::white);
            g.drawLine(startPixel, bounds.getY(), startPixel, bounds.getBottom(), 2.0f);
            g.drawLine(endPixel, bounds.getY(), endPixel, bounds.getBottom(), 2.0f);

            // Draw region between markers
            g.setColour(juce::Colour(0x30ffffff));
            g.fillRect(juce::Rectangle<float>(startPixel, float(bounds.getY()),
                                              endPixel - startPixel, float(bounds.getHeight())));
            
            // Draw marker labels
            g.setColour(juce::Colours::white);
            g.setFont(juce::Font(juce::FontOptions(12.0f)));
            g.drawText("Start", static_cast<int>(startPixel - 20), bounds.getY() - 15, 40, 15, juce::Justification::centred);
            g.drawText("End", static_cast<int>(endPixel - 20), bounds.getY() - 15, 40, 15, juce::Justification::centred);
            
            // Draw onset markers if available
            if (auto* sound = sampleManager.getSampleSound(currentSampleIndex))
            {
                g.setColour(juce::Colours::orange);
                const auto& onsetMarkers = sound->getOnsetMarkers();
                
                for (auto onsetPos : onsetMarkers)
                {
                    float onsetPixel = bounds.getX() + bounds.getWidth() * onsetPos;
                    g.drawLine(onsetPixel, bounds.getY(), onsetPixel, bounds.getBottom(), 1.5f);
                }
            }
        }
        else
        {
            // No waveform available
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            g.setFont(juce::Font(juce::FontOptions(14.0f)));
            g.drawText("Waveform not available", bounds, juce::Justification::centred);
        }
    }
    
    void mouseDown(const juce::MouseEvent& e) override
    {
        // Check if back arrow was clicked
        if (backArrowBounds.contains(e.getPosition()))
        {
            if (onBackButtonClicked)
                onBackButtonClicked();
            return;
        }

        // Check if we're near a marker
        auto bounds = getLocalBounds().reduced(10).withTrimmedTop(30);
        bounds.removeFromBottom(40); // Space for buttons

        if (!bounds.contains(e.getPosition()))
            return;

        float startPixel = bounds.getX() + bounds.getWidth() * startMarkerPosition;
        float endPixel = bounds.getX() + bounds.getWidth() * endMarkerPosition;

        const float markerTolerance = 15.0f; // Increased from 5.0f to 15.0f for easier clicking

        // Check if clicked near start marker
        if (std::abs(e.x - startPixel) < markerTolerance)
        {
            draggingStartMarker = true;
        }
        // Check if clicked near end marker
        else if (std::abs(e.x - endPixel) < markerTolerance)
        {
            draggingEndMarker = true;
        }
        // Check if clicked near an onset marker
        else if (auto* sound = sampleManager.getSampleSound(currentSampleIndex))
        {
            const auto& onsetMarkers = sound->getOnsetMarkers();
            for (size_t i = 0; i < onsetMarkers.size(); ++i)
            {
                float onsetPixel = bounds.getX() + bounds.getWidth() * onsetMarkers[i];
                if (std::abs(e.x - onsetPixel) < markerTolerance)
                {
                    draggingOnsetMarkerIndex = i;
                    break;
                }
            }
        }
    }

    void clearSampleData()
    {
        // Clear the thumbnail and reset state
        thumbnail->clear();
        currentSampleIndex = -1;
        sampleName = "No Sample";
        startMarkerPosition = 0.0;
        endMarkerPosition = 1.0;
        repaint();
    }

    void applyMarkerPositions()
    {
        if (currentSampleIndex >= 0)
        {
            // Get the sampler sound for this index
            SamplerSound* sound = sampleManager.getSampleSound(currentSampleIndex);

            if (sound != nullptr)
            {
                // Apply markers to the sound
                sound->setMarkerPositions(startMarkerPosition, endMarkerPosition);
            }
        }
    }
    
    void mouseDrag(const juce::MouseEvent& e) override
    {
        auto bounds = getLocalBounds().reduced(10).withTrimmedTop(30);
        bounds.removeFromBottom(40); // Space for buttons
        
        if (bounds.getWidth() <= 0)
            return;
            
        if (draggingStartMarker)
        {
            // Calculate normalized position (0-1)
            float pos = (e.x - bounds.getX()) / static_cast<float>(bounds.getWidth());
            pos = juce::jlimit(0.0f, endMarkerPosition - 0.01f, pos);
            
            startMarkerPosition = pos;
            repaint();
        }
        else if (draggingEndMarker)
        {
            // Calculate normalized position (0-1)
            float pos = (e.x - bounds.getX()) / static_cast<float>(bounds.getWidth());
            pos = juce::jlimit(startMarkerPosition + 0.01f, 1.0f, pos);
            
            endMarkerPosition = pos;
            repaint();
        }
        else if (draggingOnsetMarkerIndex >= 0 && currentSampleIndex >= 0)
        {
            // Update onset marker position
            if (auto* sound = sampleManager.getSampleSound(currentSampleIndex))
            {
                auto onsetMarkers = sound->getOnsetMarkers();
                if (draggingOnsetMarkerIndex < onsetMarkers.size())
                {
                    // Calculate normalized position (0-1)
                    float pos = (e.x - bounds.getX()) / static_cast<float>(bounds.getWidth());
                    pos = juce::jlimit(0.0f, 1.0f, pos);
                    
                    // Update the marker position
                    onsetMarkers[draggingOnsetMarkerIndex] = pos;
                    sound->setOnsetMarkers(onsetMarkers);
                    repaint();
                }
            }
        }
    }
    
    void mouseUp(const juce::MouseEvent&) override
    {
        if (draggingStartMarker || draggingEndMarker)
        {
            applyMarkerPositions();
        }

        draggingStartMarker = false;
        draggingEndMarker = false;
        draggingOnsetMarkerIndex = -1;
    }
    
    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
        // Repaint when thumbnail changes
        repaint();
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Position the detect onsets button at the bottom
        auto bottomArea = bounds.removeFromBottom(30);
        detectOnsetsButton->setBounds(bottomArea.reduced(10, 5));
        
        repaint();
    }
    
    float getStartMarkerPosition() const { return startMarkerPosition; }
    float getEndMarkerPosition() const { return endMarkerPosition; }
    
    void setStartMarkerPosition(float pos)
    {
        startMarkerPosition = juce::jlimit(0.0f, endMarkerPosition - 0.01f, pos);
        repaint();
    }
    
    void setEndMarkerPosition(float pos)
    {
        endMarkerPosition = juce::jlimit(startMarkerPosition + 0.01f, 1.0f, pos);
        repaint();
    }
    
    std::function<void()> onBackButtonClicked;
    
private:
    // Detect beat onsets in the current sample using approaches from academic literature
    void detectBeatOnsets()
    {
        if (currentSampleIndex < 0 || currentSampleIndex >= sampleManager.getNumSamples())
            return;
            
        auto* sound = sampleManager.getSampleSound(currentSampleIndex);
        if (!sound)
            return;
            
        // Get the audio data
        auto* audioData = sound->getAudioData();
        if (!audioData || audioData->getNumSamples() == 0)
            return;
            
        // Clear existing onset markers
        sound->clearOnsetMarkers();
        
        // Implementation based on "A Tutorial on Onset Detection in Music Signals"
        const int fftSize = 2048; // Large FFT size for good frequency resolution
        const int hopSize = fftSize / 4; // 75% overlap for better temporal accuracy
        const int minDistance = 8000; // Minimum distance between onsets
        
        int numSamples = audioData->getNumSamples();
        int numChannels = audioData->getNumChannels();
        
        // Create FFT object
        juce::dsp::FFT fft(std::log2(fftSize));
        std::vector<float> fftInput(fftSize * 2, 0.0f); // Complex input (real/imag interleaved)
        std::vector<float> window(fftSize);
        
        // Create a Hann window for better spectral analysis
        for (int i = 0; i < fftSize; ++i)
            window[i] = 0.5f - 0.5f * std::cos(2.0f * M_PI * i / (fftSize - 1));
        
        // Data structures for onset detection
        std::vector<float> energyFunction;
        std::vector<float> spectralFluxFunction;
        std::vector<float> detectionFunction;
        int numFrames = 1 + (numSamples - fftSize) / hopSize;
        
        // Pre-allocate
        energyFunction.resize(numFrames, 0.0f);
        spectralFluxFunction.resize(numFrames, 0.0f);
        detectionFunction.resize(numFrames, 0.0f);
        
        // Previous magnitudes for spectral flux calculation
        std::vector<float> prevMagnitudes(fftSize / 2 + 1, 0.0f);
        
        // Compute frame-by-frame features
        for (int frame = 0; frame < numFrames; ++frame)
        {
            // Fill FFT input with windowed audio data
            for (int i = 0; i < fftSize; ++i)
            {
                int sampleIndex = frame * hopSize + i;
                if (sampleIndex < numSamples)
                {
                    float sample = 0.0f;
                    for (int channel = 0; channel < numChannels; ++channel)
                        sample += audioData->getSample(channel, sampleIndex);
                    
                    sample /= numChannels;
                    fftInput[i * 2] = sample * window[i]; // Real part
                    fftInput[i * 2 + 1] = 0.0f;           // Imaginary part
                }
                else
                {
                    fftInput[i * 2] = 0.0f;
                    fftInput[i * 2 + 1] = 0.0f;
                }
            }
            
            // Perform FFT
            fft.performRealOnlyForwardTransform(&fftInput[0], true);
            
            // Compute energy and spectral flux
            float energy = 0.0f;
            float spectralFlux = 0.0f;
            
            for (int k = 0; k <= fftSize / 2; ++k)
            {
                // Convert complex to magnitude
                float real = fftInput[k * 2];
                float imag = fftInput[k * 2 + 1];
                float magnitude = std::sqrt(real * real + imag * imag);
                
                // Energy contribution
                energy += magnitude * magnitude;
                
                // Spectral flux (increase in magnitude)
                float diff = magnitude - prevMagnitudes[k];
                // Half-wave rectification (only consider increases)
                spectralFlux += (diff > 0.0f) ? diff : 0.0f;
                
                // Store for next frame
                prevMagnitudes[k] = magnitude;
            }
            
            // Store features
            energyFunction[frame] = energy;
            spectralFluxFunction[frame] = spectralFlux;
        }
        
        // Normalize functions
        normalize(energyFunction);
        normalize(spectralFluxFunction);
        
        // Combine detection functions (weighted sum)
        const float ENERGY_WEIGHT = 0.3f;
        const float FLUX_WEIGHT = 0.7f;
        
        for (int i = 0; i < numFrames; ++i)
            detectionFunction[i] = ENERGY_WEIGHT * energyFunction[i] + FLUX_WEIGHT * spectralFluxFunction[i];
        
        // Apply adaptive thresholding (as described in the paper)
        std::vector<float> threshold(numFrames, 0.0f);
        
        // Calculate adaptive threshold using a moving median
        const int MEDIAN_SIZE = 8; // Median filter size
        std::vector<float> medianWindow;
        
        for (int i = 0; i < numFrames; ++i)
        {
            // Fill median window
            medianWindow.clear();
            for (int j = std::max(0, i - MEDIAN_SIZE); j <= std::min(numFrames - 1, i + MEDIAN_SIZE); ++j)
                if (j != i) // Don't include the current point in the median
                    medianWindow.push_back(detectionFunction[j]);
            
            // Sort and get median
            if (!medianWindow.empty())
            {
                std::sort(medianWindow.begin(), medianWindow.end());
                threshold[i] = medianWindow[medianWindow.size() / 2];
                
                // Add a small DC offset to the threshold
                threshold[i] += 0.05f;
            }
        }
        
        // Find peaks that exceed the adaptive threshold
        std::vector<int> onsetFrames;
        int lastOnsetFrame = -minDistance;
        
        for (int i = 2; i < numFrames - 2; ++i)
        {
            // Check if it's a local maximum
            if (detectionFunction[i] > detectionFunction[i - 1] &&
                detectionFunction[i] > detectionFunction[i - 2] &&
                detectionFunction[i] > detectionFunction[i + 1] &&
                detectionFunction[i] > detectionFunction[i + 2])
            {
                // Check if it exceeds the adaptive threshold
                if (detectionFunction[i] > threshold[i] &&
                    i - lastOnsetFrame >= minDistance / hopSize)
                {
                    onsetFrames.push_back(i);
                    lastOnsetFrame = i;
                    
                    // Debug
                    DBG("Found onset at frame " + juce::String(i) + 
                        " (value: " + juce::String(detectionFunction[i]) + 
                        ", threshold: " + juce::String(threshold[i]) + ")");
                }
            }
        }
        
        // Convert frame indices to normalized positions
        for (int frame : onsetFrames)
        {
            float samplePosition = frame * hopSize;
            float normalizedPos = samplePosition / numSamples;
            sound->addOnsetMarker(normalizedPos);
            DBG("Added onset marker at position: " + juce::String(normalizedPos));
        }
        
        repaint();
    }
    
    // Utility function to normalize a vector to [0,1] range
    void normalize(std::vector<float>& data)
    {
        if (data.empty())
            return;
            
        float minValue = *std::min_element(data.begin(), data.end());
        float maxValue = *std::max_element(data.begin(), data.end());
        
        if (maxValue == minValue)
            return;
            
        for (float& value : data)
            value = (value - minValue) / (maxValue - minValue);
    }

    SampleManager& sampleManager;
    
    std::unique_ptr<juce::AudioThumbnailCache> thumbnailCache;
    std::unique_ptr<juce::AudioThumbnail> thumbnail;
    juce::AudioFormatManager formatManager;
    
    int currentSampleIndex = -1;
    juce::String sampleName;
    
    float startMarkerPosition = 0.0f;
    float endMarkerPosition = 1.0f;
    
    bool draggingStartMarker = false;
    bool draggingEndMarker = false;
    int draggingOnsetMarkerIndex = -1;
    
    juce::Path backArrowPath;
    juce::Rectangle<int> backArrowBounds;
    
    std::unique_ptr<juce::TextButton> detectOnsetsButton;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleDetailComponent)
};