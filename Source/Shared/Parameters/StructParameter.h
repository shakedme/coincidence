//
// Created by Shaked Melman on 04/04/2025.
//

#ifndef COINCIDENCE_STRUCTPARAMETER_H
#define COINCIDENCE_STRUCTPARAMETER_H


#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>
#include <memory>
#include "Parameter.h"
#include "../ModulationMatrix.h"

template<typename StructType>
class StructParameter {
public:
    struct FieldDescriptor {
        juce::Identifier paramId;
        std::function<void(StructType &, float)> setter;

        template<typename FieldType>
        FieldDescriptor(juce::Identifier id,
                        FieldType StructType::* field
        )
                : paramId(std::move(id)) {
            setter = [field](StructType &target, float value) {
                if constexpr (std::is_same_v<FieldType, float>) {
                    target.*field = value;
                } else if constexpr (std::is_same_v<FieldType, bool>) {
                    target.*field = Params::toBool(value);
                } else if constexpr (std::is_integral_v<FieldType>) {
                    target.*field = Params::toInt(value);
                } else if constexpr (std::is_enum_v<FieldType>) {
                    target.*field = static_cast<FieldType>(Params::toInt(value));
                } else {
                    jassert(false && "Unsupported field type in StructParameter");
                }
            };
        }
    };

    StructParameter(ModulationMatrix &matrix, std::vector<FieldDescriptor> descriptors, StructType defaultValue = {})
            : modulationMatrix(matrix), fieldDescriptors(std::move(descriptors)), defaultStruct(defaultValue) {}

    StructType getValue() const {
        StructType result = defaultStruct;

        for (const auto &descriptor: fieldDescriptors) {
            auto [baseValueNormalized, modValueNormalized] = modulationMatrix.getParamAndModulationValue(descriptor.paramId);
            float finalNormalizedValue = baseValueNormalized + modValueNormalized;
            finalNormalizedValue = std::clamp(finalNormalizedValue, 0.0f, 1.0f);
            descriptor.setter(result, finalNormalizedValue);
        }

        return result;
    }

private:
    ModulationMatrix &modulationMatrix;
    std::vector<FieldDescriptor> fieldDescriptors;
    StructType defaultStruct;
};

template<typename StructType, typename FieldType>
typename StructParameter<StructType>::FieldDescriptor
makeFieldDescriptor(juce::Identifier paramId,
                    FieldType StructType::* field
) {
    return typename StructParameter<StructType>::FieldDescriptor(paramId, field);
}

#endif //COINCIDENCE_STRUCTPARAMETER_H
