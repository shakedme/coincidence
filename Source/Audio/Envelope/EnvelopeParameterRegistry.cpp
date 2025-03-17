#include "EnvelopeParameterTypes.h"
#include <stdexcept>

namespace EnvelopeParams {
    // Constructor initializes the registry
    Registry::Registry() {
        initialize();
    }
    
    const std::vector<EnvelopeTypeInfo>& Registry::getAvailableTypes() const {
        return types;
    }
    
    EnvelopeTypeInfo Registry::getTypeInfo(ParameterType type) const {
        // Find type in the registry
        for (const auto& info : types) {
            if (info.type == type) {
                return info;
            }
        }
        
        // Return default if not found
        throw std::runtime_error("Envelope parameter type not found in registry");
    }
    
    void Registry::registerType(const EnvelopeTypeInfo& typeInfo) {
        // Check if type already exists
        for (size_t i = 0; i < types.size(); i++) {
            if (types[i].type == typeInfo.type) {
                // Update existing
                types[i] = typeInfo;
                return;
            }
        }
        
        // Add new type
        types.push_back(typeInfo);
    }
    
    void Registry::initialize() {
        // Clear existing types and register defaults
        types.clear();
        
        // Register default envelope types
        registerType({
            ParameterType::Amplitude,
            "Amplitude",
            {0.0f, 1.0f, false, 1.0f, false},
            true, // visible
            true  // affects audio
        });
        
        registerType({
            ParameterType::Reverb,
            "Reverb",
            {0.0f, 1.0f, false, 0.0f, false},
            true, // visible
            false // doesn't directly affect audio (handled by effects engine)
        });
        
        registerType({
            ParameterType::Delay,
            "Delay",
            {0.0f, 1.0f, false, 0.0f, false},
            true, // visible
            false // doesn't directly affect audio (handled by effects engine)
        });
    }
} 