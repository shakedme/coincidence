#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

class FDNReverb
{
public:

    void prepare(double sampleRate, int maxBlockSize)
    {
        this->sampleRate = sampleRate;
        for (int i = 0; i < numDelays; ++i)
        {
            delayBuffers[i].setSize(1, maxBlockSize + delayTimes[i]);
            delayPositions[i] = 0;
            modPhases[i] = 0.0f;
        }
    }

    void setDecay(float newDecay)
    {
        decay = juce::jlimit(0.0f, 1.0f, newDecay);
        for (int i = 0; i < numDelays; ++i)
            feedbackGain[i] = juce::jmap(decay, 0.0f, 1.0f, 0.5f, 0.9f);
    }

    void processStereo(float* left, float* right, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float input = (left[i] + right[i]) * gain;
            float outL = 0, outR = 0;

            // Early reflections: 4-tap delay network
            float earlyReflections = 0.0f;
            for (int j = 0; j < numEarlyTaps; ++j)
                earlyReflections += earlyTapGains[j] * delayRead(earlyDelays[j]);

            // Feedback Delay Network (FDN)
            float fdnOut[numDelays] = {};
            for (int j = 0; j < numDelays; ++j)
            {
                float modOffset = 2.0f * sinf(modPhases[j]);
                modPhases[j] += modRates[j];
                if (modPhases[j] > juce::MathConstants<float>::twoPi)
                    modPhases[j] -= juce::MathConstants<float>::twoPi;

                float delaySample = delayRead(delayTimes[j] + modOffset);
                fdnOut[j] = delaySample * feedbackGain[j];

                // Apply damping filter (high frequency absorption)
                float dampedSample =
                    lowpassFilters[j].processSingleSampleRaw(delaySample);
                delayWrite(j, input + earlyReflections + dampedSample);
            }

            // Matrix mixing (Householder transform)
            float mixedL = 0, mixedR = 0;
            for (int j = 0; j < numDelays; ++j)
            {
                mixedL += fdnMatrixL[j] * fdnOut[j];
                mixedR += fdnMatrixR[j] * fdnOut[j];
            }

            // Mix wet/dry
            left[i] = mixedL * wetGain + left[i] * dryGain;
            right[i] = mixedR * wetGain + right[i] * dryGain;
        }
    }

private:
    static constexpr int numDelays = 8;
    static constexpr int numEarlyTaps = 4;
    float delayTimes[numDelays] = {151, 163, 173, 191, 211, 223, 241, 263};
    float earlyDelays[numEarlyTaps] = {23, 41, 67, 89};
    float earlyTapGains[numEarlyTaps] = {0.3f, 0.4f, 0.5f, 0.6f};
    float feedbackGain[numDelays] = {
        0.72f, 0.73f, 0.71f, 0.74f, 0.75f, 0.70f, 0.76f, 0.77f};
    float fdnMatrixL[numDelays] = {-0.5, 0.5, -0.5, 0.5, -0.5, 0.5, -0.5, 0.5};
    float fdnMatrixR[numDelays] = {0.5, -0.5, 0.5, -0.5, 0.5, -0.5, 0.5, -0.5};

    juce::AudioBuffer<float> delayBuffers[numDelays];
    int delayPositions[numDelays] = {0};
    juce::IIRFilter lowpassFilters[numDelays];
    float modPhases[numDelays] = {0};
    float modRates[numDelays] = {0.02f, 0.03f, 0.04f, 0.05f, 0.06f, 0.07f, 0.08f, 0.09f};

    float wetGain = 0.6f, dryGain = 0.4f, gain = 0.8f;
    float decay = 0.75f;
    double sampleRate;

    void delayWrite(int index, float value)
    {
        delayBuffers[index].setSample(0, delayPositions[index], value);
        delayPositions[index] =
            (delayPositions[index] + 1) % delayBuffers[index].getNumSamples();
    }

    float delayRead(float offset)
    {
        int index = juce::roundToInt(offset) % delayBuffers[0].getNumSamples();
        return delayBuffers[0].getSample(0, index);
    }
};
