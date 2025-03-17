#pragma once

namespace EnvelopeParams {
    enum class ParameterType {
        Amplitude,
        Reverb,
        Delay
        // Add more parameter types here as needed
    };


    struct ParameterSettings {
        float minValue = 0.0f;
        float maxValue = 1.0f;
        bool exponential = false;
        float defaultValue = 0.5f;
        bool bipolar = false;  // For parameters that can go negative
    };

    // Factory function to get default settings for each parameter type
    inline ParameterSettings getDefaultSettings(ParameterType type) {
        switch (type) {
            case ParameterType::Amplitude:
                return {0.0f, 1.0f, false, 0.5f, false};
            case ParameterType::Reverb:
                return {0.0f, 1.0f, false, 0.5f, false};
            case ParameterType::Delay:
                return {0.0f, 1.0f, false, 0.5f, false};
            default:
                return {0.0f, 1.0f, false, 0.5f, false};
        }
    }
} 