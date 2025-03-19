#pragma once

#include <vector>
#include <string>
#include <map>

namespace EnvelopeParams {
    enum class ParameterType {
        Amplitude,
        Reverb
    };

    struct ParameterSettings {
        float minValue = 0.0f;
        float maxValue = 1.0f;
        bool exponential = false;
        float defaultValue = 0.5f;
        bool bipolar = false;
    };

    struct EnvelopeTypeInfo {
        ParameterType type;
        juce::Identifier id;
        std::string name;
        ParameterSettings settings;
    };

    inline std::vector<EnvelopeTypeInfo> getAvailableTypes() {
        return {
                {ParameterType::Amplitude, AppState::ID_AMPLITUDE_ENVELOPE, "Amplitude", {0.0f, 1.0f, false, 0.5f, false}},
                {ParameterType::Reverb,    AppState::ID_REVERB_ENV,         "Reverb",    {0.0f, 1.0f, false, 0.5f, false}},
        };
    };

} 