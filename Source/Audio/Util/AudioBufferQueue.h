//
// Created by Shaked Melman on 11/03/2025.
//

#ifndef COINCIDENCE_AUDIOBUFFERQUEUE_H
#define COINCIDENCE_AUDIOBUFFERQUEUE_H

#include <array>
#include <algorithm>
#include <mutex>

class AudioBufferQueue {
public:
    static constexpr size_t capacity = 44100 * 5; // 5 seconds at 44.1kHz

    AudioBufferQueue() {
        // Clear the buffer
        std::fill(std::begin(buffer), std::end(buffer), 0.0f);
    }

    // Write new samples to the ring buffer (lock-free, called from audio thread)
    void push(const float *data, size_t numSamples) {
        const size_t numToWrite = std::min(numSamples, capacity);

        // Check for wrap-around
        if (writePos + numToWrite > capacity) {
            // Handle wrap-around by writing in two parts
            const size_t firstPart = capacity - writePos;
            const size_t secondPart = numToWrite - firstPart;

            // Copy first part
            std::copy(data, data + firstPart, buffer.begin() + writePos);

            // Copy second part at the beginning of the buffer
            std::copy(data + firstPart, data + numToWrite, buffer.begin());

            writePos = secondPart;
        } else {
            // Simple copy without wrap-around
            std::copy(data, data + numToWrite, buffer.begin() + writePos);
            writePos = (writePos + numToWrite) % capacity;
        }

        // Update total samples written
        totalSamplesWritten += numToWrite;
    }

    // Get the total number of samples written so far
    size_t getTotalSamplesWritten() const {
        return totalSamplesWritten;
    }

    // Get samples relative to the current write position
    void getVisibleSamples(float *destination, size_t numSamples, size_t offset) {
        // Calculate start position (going backwards from write position)
        // We need to handle wrap-around
        size_t startPosition = 0;

        if (writePos >= offset + numSamples) {
            startPosition = writePos - offset - numSamples;
        } else {
            // Handle wrap-around - we need to go backwards from writePos
            size_t wrapAmount = offset + numSamples - writePos;
            startPosition = capacity - wrapAmount;
        }

        // Make sure we don't exceed total samples written
        size_t maxSamples = std::min(numSamples, static_cast<size_t>(totalSamplesWritten));

        // Handle wrap-around in the copying
        if (startPosition + maxSamples > capacity) {
            size_t firstPartSize = capacity - startPosition;
            size_t secondPartSize = maxSamples - firstPartSize;

            // Copy first part from end of buffer
            std::copy(buffer.begin() + startPosition, buffer.begin() + capacity, destination);

            // Copy second part from beginning of buffer
            std::copy(buffer.begin(), buffer.begin() + secondPartSize, destination + firstPartSize);
        } else {
            // Straight copy without wrap-around
            std::copy(buffer.begin() + startPosition, buffer.begin() + startPosition + maxSamples, destination);
        }
    }

private:
    std::array<float, capacity> buffer;
    size_t writePos = 0;
    size_t totalSamplesWritten = 0;

};

#endif //COINCIDENCE_AUDIOBUFFERQUEUE_H
