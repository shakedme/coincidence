#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../../Audio/SampleManager.h"

class SampleDetailComponent : public juce::Component,
                              private juce::ChangeListener
{
public:
    SampleDetailComponent(SampleManager& sampleManager)
        : sampleManager(sampleManager)
    {
        // Initialize the thumbnail cache and source
        thumbnailCache = std::make_unique<juce::AudioThumbnailCache>(5);
        thumbnail = std::make_unique<juce::AudioThumbnail>(512, formatManager, *thumbnailCache);

        formatManager.registerBasicFormats();

        startMarkerPosition = 0.0;
        endMarkerPosition = 1.0;

        // Start listening for changes to the thumbnail
        thumbnail->addChangeListener(this);

        // Add back button
        backButton = std::make_unique<juce::TextButton>("Back to List");
        backButton->onClick = [this]() {
            if (onBackButtonClicked)
                onBackButtonClicked();
        };
        addAndMakeVisible(backButton.get());
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

            if (currentSampleIndex >= 0 && currentSampleIndex < sampleManager.getNumSamples())
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

        // Draw sample name
        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        g.drawText(sampleName, getLocalBounds().removeFromTop(30), juce::Justification::centred, true);

        auto bounds = getLocalBounds().reduced(10).withTrimmedTop(30);
        bounds.removeFromBottom(40); // Space for buttons

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
            g.setFont(12.0f);
            g.drawText("Start", startPixel - 20, bounds.getY() - 15, 40, 15, juce::Justification::centred);
            g.drawText("End", endPixel - 20, bounds.getY() - 15, 40, 15, juce::Justification::centred);
        }
        else
        {
            // No waveform available
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            g.setFont(14.0f);
            g.drawText("Waveform not available", bounds, juce::Justification::centred);
        }
    }
    
    void mouseDown(const juce::MouseEvent& e) override
    {
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
    }
    
    void mouseUp(const juce::MouseEvent&) override
    {
        if (draggingStartMarker || draggingEndMarker)
        {
            applyMarkerPositions();
        }

        draggingStartMarker = false;
        draggingEndMarker = false;
    }
    
    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
        // Repaint when thumbnail changes
        repaint();
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Position back button at the bottom
        auto buttonArea = bounds.removeFromBottom(40);
        backButton->setBounds(buttonArea.withSizeKeepingCentre(120, 30));
        
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
    
    std::unique_ptr<juce::TextButton> backButton;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleDetailComponent)
};