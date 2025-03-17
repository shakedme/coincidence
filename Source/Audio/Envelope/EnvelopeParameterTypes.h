#pragma once

#include <vector>
#include <string>
#include <map>

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

    // Enhanced type information for envelope parameters
    struct EnvelopeTypeInfo {
        ParameterType type;
        std::string name;
        ParameterSettings settings;
        bool visible = true;    // Whether it should be shown in UI
        bool affectsAudio = false; // Whether it directly affects audio processing
    };

    // Registry of all available envelope types - now instance-based
    class Registry {
    public:
        // Constructor initializes with default types
        Registry();

        // Get all available types
        [[nodiscard]] const std::vector<EnvelopeTypeInfo> &getAvailableTypes() const;

        // Get info for a specific type
        [[nodiscard]] EnvelopeTypeInfo getTypeInfo(ParameterType type) const;

        // Register or update a type
        void registerType(const EnvelopeTypeInfo &typeInfo);

    private:
        // Initialize with default envelope types
        void initialize();

        // Member variable to store types (no longer static)
        std::vector<EnvelopeTypeInfo> types;
    };

} 