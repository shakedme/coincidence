#include "WaveformComponent.h"

WaveformComponent::WaveformComponent()
{
    setInterceptsMouseClicks(false, true);
    // Set up waveform rendering
    setupWaveformRendering();
    
    // Start the timer for waveform updates - use higher refresh rate for smoother animation
    startTimerHz(30); // 30 FPS for smoother visualization
}

WaveformComponent::~WaveformComponent()
{
    stopTimer();
    
    // Clean up the audio buffer queue
    if (audioBufferQueue != nullptr)
    {
        delete audioBufferQueue;
        audioBufferQueue = nullptr;
    }
}

void WaveformComponent::paint(juce::Graphics& g)
{
    // Draw background
    g.fillAll(backgroundColour);
    
    // Draw waveform
    if (waveformCache.isValid())
    {
        g.drawImageAt(waveformCache, 0, 0);
    }
}

void WaveformComponent::resized()
{
    // Update waveform data buffer when component is resized
    if (waveformData.size() != static_cast<size_t>(getWidth()) && getWidth() > 0)
    {
        waveformData.resize(getWidth(), 0.0f);
        waveformPeaks.resize(getWidth());
        waveformCache = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
        waveformNeedsRedraw.store(true);
    }
}

void WaveformComponent::setSampleRate(float newSampleRate)
{
    sampleRate = newSampleRate;
}

void WaveformComponent::setTimeRange(float seconds)
{
    timeRangeInSeconds = seconds;
    waveformNeedsRedraw.store(true);
}

void WaveformComponent::setWaveformScaleFactor(float scale)
{
    waveformScaleFactor = scale;
    waveformNeedsRedraw.store(true);
}

void WaveformComponent::setWaveformColour(juce::Colour colour)
{
    waveformColour = colour;
    waveformNeedsRedraw.store(true);
}

void WaveformComponent::setBackgroundColour(juce::Colour colour)
{
    backgroundColour = colour;
    repaint();
}

void WaveformComponent::setWaveformAlpha(float alpha)
{
    waveformAlpha = juce::jlimit(0.0f, 1.0f, alpha);
    waveformNeedsRedraw.store(true);
}

void WaveformComponent::pushAudioBuffer(const float* audioData, int numSamples)
{
    // This method is called from the audio thread
    // Add data to the ring buffer in a lock-free manner
    if (audioBufferQueue != nullptr)
    {
        audioBufferQueue->push(audioData, numSamples);
        waveformNeedsRedraw.store(true);
    }
}

void WaveformComponent::setupWaveformRendering()
{
    // Create the buffer queue
    audioBufferQueue = new AudioBufferQueue();
    
    // Initialize waveform data buffer based on component width
    waveformData.resize(getWidth() > 0 ? getWidth() : 300, 0.0f);
    waveformPeaks.resize(waveformData.size());
    
    // Create the initial cache image
    waveformCache = juce::Image(juce::Image::ARGB, 
                                getWidth() > 0 ? getWidth() : 300, 
                                getHeight() > 0 ? getHeight() : 150, 
                                true);
}

void WaveformComponent::updateWaveformCache()
{
    // Check if component has been resized
    if (waveformData.size() != static_cast<size_t>(getWidth()) && getWidth() > 0)
    {
        waveformData.resize(getWidth(), 0.0f);
        waveformPeaks.resize(getWidth());
        waveformCache = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
    }
    
    // Fetch the audio data for the visible time range
    if (audioBufferQueue != nullptr)
    {
        // Calculate how many samples we need based on the time range
        size_t samplesForTimeRange = static_cast<size_t>(timeRangeInSeconds * sampleRate);
        
        // Resize waveformData if necessary to match time range
        if (waveformData.size() != samplesForTimeRange && samplesForTimeRange > 0)
        {
            waveformData.resize(samplesForTimeRange, 0.0f);
        }
        
        // Get samples for the visible range - we want the most recent samples, so offset is 0
        audioBufferQueue->getVisibleSamples(waveformData.data(), waveformData.size(), 0);
    }
    
    // Prepare the peak data structure
    waveformPeaks.resize(getWidth());
    
    // Process the raw sample data into min-max peaks for each pixel column
    const float samplesPerPixel = static_cast<float>(waveformData.size()) / getWidth();
    
    // For each horizontal pixel in our display
    for (int x = 0; x < getWidth(); ++x)
    {

        // For left-to-right flow, we need to reverse the x index used to look up samples
        // This makes newest samples appear on the right and older samples flow left
        int reverseX = getWidth() - 1 - x;

        // Calculate the range of samples that correspond to this pixel
        const int startSample = static_cast<int>(reverseX * samplesPerPixel);
        const int endSample = static_cast<int>((reverseX + 1) * samplesPerPixel);

        // Find min and max values in this range
        float minValue = 0.0f;
        float maxValue = 0.0f;
        
        // Make sure we don't go out of bounds
        const int numSamples = std::min(endSample, static_cast<int>(waveformData.size()));
        
        for (int sample = startSample; sample < numSamples; ++sample)
        {
            const float value = waveformData[sample];
            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
        }
        
        // Store the peak data
        waveformPeaks[x].min = minValue * waveformScaleFactor;
        waveformPeaks[x].max = maxValue * waveformScaleFactor;
    }
    
    // Draw the waveform to the cache image
    juce::Graphics g(waveformCache);
    g.fillAll(juce::Colours::transparentBlack);

    // Draw waveform peaks with the specified alpha
    g.setColour(waveformColour.withAlpha(waveformAlpha));
    
    for (int x = 0; x < static_cast<int>(waveformPeaks.size()) && x < getWidth(); ++x)
    {
        // Calculate the y positions for min and max values
        const float minY = juce::jmap(waveformPeaks[x].min, -1.0f, 1.0f, 
                                     static_cast<float>(getHeight()), 0.0f);
        const float maxY = juce::jmap(waveformPeaks[x].max, -1.0f, 1.0f, 
                                     static_cast<float>(getHeight()), 0.0f);
        
        // Draw a vertical line from min to max
        g.drawLine(static_cast<float>(x), minY, static_cast<float>(x), maxY, 1.0f);
    }
}

void WaveformComponent::timerCallback()
{
    // Check if we need to update the waveform
    if (waveformNeedsRedraw.load())
    {
        // Use mutex to protect the cache update, but not the data acquisition
        std::lock_guard<std::mutex> lock(waveformMutex);
        updateWaveformCache();
        waveformNeedsRedraw.store(false);
        repaint();
    }
} 