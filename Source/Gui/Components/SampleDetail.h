#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "../../Audio/Sampler/SampleManager.h"
#include "../../Audio/Sampler/OnsetDetector.h"

class SampleDetailComponent
    : public juce::Component
    , private juce::ChangeListener
    , public juce::KeyListener
{
public:
    SampleDetailComponent(SampleManager& manager)
        : sampleManager(manager)
    {
        // Initialize the thumbnail cache and source
        thumbnailCache = std::make_unique<juce::AudioThumbnailCache>(5);
        thumbnail =
            std::make_unique<juce::AudioThumbnail>(4096, formatManager, *thumbnailCache);

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
        
        // Add a button to clear all onset markers
        clearMarkersButton = std::make_unique<juce::TextButton>("Clear Markers");
        clearMarkersButton->onClick = [this]() { 
            if (auto* sound = sampleManager.getSampleSound(currentSampleIndex))
            {
                sound->clearOnsetMarkers();
                repaint();
            }
        };
        addAndMakeVisible(clearMarkersButton.get());
        
        // Enable keyboard focus for this component to receive key presses
        setWantsKeyboardFocus(true);
        addKeyListener(this);
    }

    ~SampleDetailComponent() override { 
        thumbnail->removeChangeListener(this); 
        removeKeyListener(this);
    }

    int getSampleIndex() { return currentSampleIndex; }

    void setSampleIndex(int index)
    {
        if (currentSampleIndex != index)
        {
            currentSampleIndex = index;

            if (currentSampleIndex >= 0
                && currentSampleIndex < sampleManager.getNumSamples())
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

                    thumbnail->setSource(&audioData, sampleRate, static_cast<juce::int64>(currentSampleIndex));

//                    // Set the sample data to the thumbnail
//                    thumbnail->reset(audioData.getNumChannels(),
//                                     sampleRate,
//                                     audioData.getNumSamples());
//
//                    // Add the sample data to the thumbnail
//                    thumbnail->addBlock(0, audioData, 0, audioData.getNumSamples());

                    // If no onset markers exist, detect them
                    if (sound->getOnsetMarkers().empty())
                    {
                        detectOnsets();
                    }

                    repaint();
                }
            }
        }
    }

    // Method to rebuild the waveform without changing the current sample
    void rebuildWaveform()
    {
        if (currentSampleIndex >= 0 && currentSampleIndex < sampleManager.getNumSamples())
        {
            // Get the sample sound
            if (auto* sound = sampleManager.getSampleSound(currentSampleIndex))
            {
                // Clear previous thumbnail
                thumbnail->clear();

                // Create a reader for the sample data
                auto& audioData = *sound->getAudioData();
                double sampleRate = sound->getSourceSampleRate();

                // Set the sample data to the thumbnail
                thumbnail->reset(audioData.getNumChannels(),
                                 sampleRate,
                                 audioData.getNumSamples());

                // Add the sample data to the thumbnail
                thumbnail->addBlock(0, audioData, 0, audioData.getNumSamples());

                repaint();
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
        g.drawText(sampleName,
                   getLocalBounds().removeFromTop(30),
                   juce::Justification::centred,
                   true);

        auto bounds = getLocalBounds().reduced(10).withTrimmedTop(30);
        // Reserve space for clearMarkersButton
        bounds.removeFromBottom(40);

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
            g.fillRect(juce::Rectangle<float>(startPixel,
                                              float(bounds.getY()),
                                              endPixel - startPixel,
                                              float(bounds.getHeight())));

            // Draw marker labels
            g.setColour(juce::Colours::white);
            g.setFont(juce::Font(juce::FontOptions(12.0f)));
            g.drawText("Start",
                       static_cast<int>(startPixel - 20),
                       bounds.getY() - 15,
                       40,
                       15,
                       juce::Justification::centred);
            g.drawText("End",
                       static_cast<int>(endPixel - 20),
                       bounds.getY() - 15,
                       40,
                       15,
                       juce::Justification::centred);

            // Draw onset markers
            if (auto* sound = sampleManager.getSampleSound(currentSampleIndex))
            {
                const auto& onsetMarkers = sound->getOnsetMarkers();

                for (int i = 0; i < onsetMarkers.size(); ++i)
                {
                    float onsetPosition = onsetMarkers[i];
                    float onsetPixel = bounds.getX() + bounds.getWidth() * onsetPosition;

                    // Use a different color if this marker is selected
                    if (i == selectedMarkerIndex) {
                        g.setColour(juce::Colours::red); // Selected marker is red
                    } else {
                        g.setColour(juce::Colour(0xff52bfd9)); // Blue color for onset markers
                    }

                    // Draw a thinner line for onset markers
                    g.drawLine(
                        onsetPixel, bounds.getY(), onsetPixel, bounds.getBottom(), 1.0f);

                    // Draw a downward-facing triangle at the top of the marker
                    float triangleSize = 8.0f; // Slightly larger than the circle
                    juce::Path trianglePath;
                    trianglePath.startNewSubPath(onsetPixel, bounds.getY() + triangleSize);
                    trianglePath.lineTo(onsetPixel - triangleSize/2, bounds.getY());
                    trianglePath.lineTo(onsetPixel + triangleSize/2, bounds.getY());
                    trianglePath.closeSubPath();
                    g.fillPath(trianglePath);
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

        const float markerTolerance =
            15.0f; // Increased from 5.0f to 15.0f for easier clicking

        // Check if clicked near start marker
        if (std::abs(e.x - startPixel) < markerTolerance)
        {
            draggingStartMarker = true;
            selectedMarkerIndex = -1; // Deselect any selected onset marker
        }
        // Check if clicked near end marker
        else if (std::abs(e.x - endPixel) < markerTolerance)
        {
            draggingEndMarker = true;
            selectedMarkerIndex = -1; // Deselect any selected onset marker
        }
        // Check if clicked near an onset marker
        else if (auto* sound = sampleManager.getSampleSound(currentSampleIndex))
        {
            const auto& onsetMarkers = sound->getOnsetMarkers();

            // Find the closest onset marker
            currentOnsetMarkerIndex = findClosestOnsetMarker(e.x, bounds);

            if (currentOnsetMarkerIndex >= 0
                && currentOnsetMarkerIndex < onsetMarkers.size())
            {
                float onsetPixel =
                    bounds.getX()
                    + bounds.getWidth() * onsetMarkers[currentOnsetMarkerIndex];

                if (std::abs(e.x - onsetPixel) < markerTolerance)
                {
                    if (e.mods.isShiftDown() || e.mods.isAltDown()) {
                        // Allow changing selected marker without dragging
                        selectedMarkerIndex = currentOnsetMarkerIndex;
                        repaint();
                    } else {
                        // Normal behavior: allow dragging and select the marker
                        draggingOnsetMarker = true;
                        selectedMarkerIndex = currentOnsetMarkerIndex;
                    }
                }
                else {
                    // Clicked away from markers, clear selection
                    selectedMarkerIndex = -1;
                }
            }
            else {
                // Clicked away from markers, clear selection
                selectedMarkerIndex = -1;
            }
            repaint();
        }

        // Double-click to add a new onset marker
        if (e.getNumberOfClicks() >= 2 && !draggingStartMarker && !draggingEndMarker
            && !draggingOnsetMarker)
        {
            if (auto* sound = sampleManager.getSampleSound(currentSampleIndex))
            {
                // Convert pixel position to normalized position
                float newMarkerPos = pixelToPosition(e.x, bounds);

                // Add new onset marker
                std::vector<float> markers = sound->getOnsetMarkers();
                markers.push_back(newMarkerPos);

                // Sort markers for consistent display
                std::sort(markers.begin(), markers.end());

                // Update the sound
                sound->setOnsetMarkers(markers);

                // Find and select the newly added marker
                auto it = std::find(markers.begin(), markers.end(), newMarkerPos);
                if (it != markers.end()) {
                    selectedMarkerIndex = std::distance(markers.begin(), it);
                }

                repaint();
            }
        }
        
        // Make sure this component gets keyboard focus when clicked
        grabKeyboardFocus();
    }

    void clearSampleData()
    {
        // Clear the thumbnail and reset state
        thumbnail->clear();
        currentSampleIndex = -1;
        sampleName = "No Sample";
        startMarkerPosition = 0.0;
        endMarkerPosition = 1.0;
        selectedMarkerIndex = -1;
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
        else if (draggingOnsetMarker && currentOnsetMarkerIndex >= 0)
        {
            if (auto* sound = sampleManager.getSampleSound(currentSampleIndex))
            {
                std::vector<float> markers = sound->getOnsetMarkers();

                if (currentOnsetMarkerIndex < markers.size())
                {
                    // Calculate normalized position (0-1)
                    float pos =
                        (e.x - bounds.getX()) / static_cast<float>(bounds.getWidth());
                    pos = juce::jlimit(0.0f, 1.0f, pos);

                    // Update marker position
                    markers[currentOnsetMarkerIndex] = pos;

                    // Sort to keep markers in order
                    std::sort(markers.begin(), markers.end());

                    // Find the new index of our marker
                    auto it = std::find(markers.begin(), markers.end(), pos);
                    if (it != markers.end())
                    {
                        currentOnsetMarkerIndex = std::distance(markers.begin(), it);
                        selectedMarkerIndex = currentOnsetMarkerIndex; // Keep the marker selected
                    }

                    // Update markers
                    sound->setOnsetMarkers(markers);

                    repaint();
                }
            }
        }
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (draggingStartMarker || draggingEndMarker)
        {
            applyMarkerPositions();
        }

        // If right-click on an onset marker, ask if user wants to delete it
        if (e.mods.isRightButtonDown() && !draggingStartMarker && !draggingEndMarker
            && !draggingOnsetMarker)
        {
            auto bounds = getLocalBounds().reduced(10).withTrimmedTop(30);
            bounds.removeFromBottom(40); // Space for buttons

            if (bounds.contains(e.getPosition()))
            {
                if (auto* sound = sampleManager.getSampleSound(currentSampleIndex))
                {
                    // Find the closest onset marker
                    int closestIndex = findClosestOnsetMarker(e.x, bounds);

                    if (closestIndex >= 0)
                    {
                        const auto& markers = sound->getOnsetMarkers();
                        float onsetPixel =
                            bounds.getX() + bounds.getWidth() * markers[closestIndex];

                        const float markerTolerance = 15.0f;
                        if (std::abs(e.x - onsetPixel) < markerTolerance)
                        {
                            // Create popup menu with delete option
                            juce::PopupMenu menu;
                            menu.addItem(1, "Delete Onset Marker");

                            menu.showMenuAsync(
                                juce::PopupMenu::Options().withTargetComponent(this),
                                [this, closestIndex](int result)
                                {
                                    if (result == 1) // Delete
                                    {
                                        if (auto* sound = sampleManager.getSampleSound(
                                                currentSampleIndex))
                                        {
                                            std::vector<float> markers =
                                                sound->getOnsetMarkers();
                                            if (closestIndex < markers.size())
                                            {
                                                markers.erase(markers.begin()
                                                              + closestIndex);
                                                sound->setOnsetMarkers(markers);
                                                
                                                // Clear selection if the deleted marker was selected
                                                if (selectedMarkerIndex == closestIndex) {
                                                    selectedMarkerIndex = -1;
                                                }
                                                // Adjust selection index if a marker before the selection was deleted
                                                else if (selectedMarkerIndex > closestIndex) {
                                                    selectedMarkerIndex--;
                                                }
                                                
                                                repaint();
                                            }
                                        }
                                    }
                                });
                        }
                    }
                }
            }
        }

        draggingStartMarker = false;
        draggingEndMarker = false;
        draggingOnsetMarker = false;
        currentOnsetMarkerIndex = -1;
    }
    
    // Implementation for KeyListener
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override
    {
        // Handle Backspace key to delete the selected marker
        if (key.isKeyCode(juce::KeyPress::backspaceKey) && selectedMarkerIndex >= 0)
        {
            if (auto* sound = sampleManager.getSampleSound(currentSampleIndex))
            {
                std::vector<float> markers = sound->getOnsetMarkers();
                if (selectedMarkerIndex < markers.size())
                {
                    markers.erase(markers.begin() + selectedMarkerIndex);
                    sound->setOnsetMarkers(markers);
                    selectedMarkerIndex = -1; // Clear selection after deletion
                    repaint();
                    return true; // Key was handled
                }
            }
        }
        return false; // Key wasn't handled
    }
    
    bool keyStateChanged(bool isKeyDown, juce::Component* originatingComponent) override
    {
        return false; // We only care about keyPressed
    }

    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
        // Repaint when thumbnail changes
        repaint();
    }

    void resized() override
    { 
        auto bounds = getLocalBounds();
        bounds.removeFromTop(bounds.getHeight() - 30); // Position at bottom
        clearMarkersButton->setBounds(bounds.reduced(10, 0));
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

    bool draggingOnsetMarker = false;
    int currentOnsetMarkerIndex = -1;
    int selectedMarkerIndex = -1; // Track which marker is currently selected

    juce::Path backArrowPath;
    juce::Rectangle<int> backArrowBounds;
    
    std::unique_ptr<juce::TextButton> clearMarkersButton;

    float pixelToPosition(int x, const juce::Rectangle<int>& bounds)
    {
        float position = (x - bounds.getX()) / static_cast<float>(bounds.getWidth());
        return juce::jlimit(0.0f, 1.0f, position);
    }

    int positionToPixel(float position, const juce::Rectangle<int>& bounds)
    {
        return bounds.getX() + static_cast<int>(position * bounds.getWidth());
    }

    int findClosestOnsetMarker(int x, const juce::Rectangle<int>& bounds)
    {
        if (auto* sound = sampleManager.getSampleSound(currentSampleIndex))
        {
            const auto& onsetMarkers = sound->getOnsetMarkers();
            int closestIndex = -1;
            float closestDistance = std::numeric_limits<float>::max();

            for (int i = 0; i < onsetMarkers.size(); ++i)
            {
                float onsetPixel = bounds.getX() + bounds.getWidth() * onsetMarkers[i];
                float distance = std::abs(x - onsetPixel);

                if (distance < closestDistance)
                {
                    closestDistance = distance;
                    closestIndex = i;
                }
            }

            return closestIndex;
        }

        return -1;
    }

    void detectOnsets()
    {
        if (auto* sound = sampleManager.getSampleSound(currentSampleIndex))
        {
            // Create an onset detector
            OnsetDetector detector;

            // Get audio data
            auto& audioData = *sound->getAudioData();
            double sampleRate = sound->getSourceSampleRate();
            // Set the onset markers
            sound->setOnsetMarkers(detector.detectOnsets(audioData, sampleRate));
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleDetailComponent)
};