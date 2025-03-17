#include "Delay.h"

Delay::Delay()
        : BaseEffect() {
}

void Delay::initialize(PluginProcessor &p) {
    BaseEffect::initialize(p);

    // Create parameter binding
    paramBinding = AppState::createParameterBinding<Models::DelaySettings>(settings, p.getAPVTS());
    paramBinding->registerParameters(AppState::createDelayParameters());
}

Delay::~Delay() {
    // Cleanup resources if needed
}

void Delay::prepare(const juce::dsp::ProcessSpec &spec) {
    // Call the base class prepare
    BaseEffect::prepare(spec);

    // Prepare delay lines
    delayLineLeft.prepare(spec);
    delayLineRight.prepare(spec);

    // Reset the delay lines
    reset();
}

void Delay::process(const juce::dsp::ProcessContextReplacing<float> &context) {
    // Check if effect should be applied
    if (settings.delayMix > 0.01f) {
        auto &inputBlock = context.getInputBlock();
        auto &outputBlock = context.getOutputBlock();

        // Calculate delay time in samples
        auto delayTimeSamples = static_cast<float>(sampleRate * settings.delayRate);
        delayLineLeft.setDelay(delayTimeSamples);
        delayLineRight.setDelay(delayTimeSamples);

        // Set feedback level
        delayFeedback = settings.delayFeedback;

        // Process each channel
        for (size_t channel = 0; channel < inputBlock.getNumChannels(); ++channel) {
            auto *channelData = outputBlock.getChannelPointer(channel);

            for (size_t sample = 0; sample < inputBlock.getNumSamples(); ++sample) {
                float inputSample = channelData[sample];
                float delaySample;

                // Get delayed sample from the appropriate delay line
                if (channel == 0)
                    delaySample = delayLineLeft.popSample(0);
                else
                    delaySample = delayLineRight.popSample(0);

                // Mix input with delayed signal
                float outputSample = inputSample + (delaySample * settings.delayMix);

                // Push the combined signal back into the delay line with feedback
                if (channel == 0)
                    delayLineLeft.pushSample(0, inputSample + (delaySample * delayFeedback));
                else
                    delayLineRight.pushSample(0, inputSample + (delaySample * delayFeedback));

                // Write to output
                channelData[sample] = outputSample;
            }
        }
    }
}

void Delay::reset() {
    BaseEffect::reset();
    delayLineLeft.reset();
    delayLineRight.reset();
}