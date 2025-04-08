//
// Created by Shaked Melman on 03/04/2025.
//

#ifndef COINCIDENCE_PARAMETER_H
#define COINCIDENCE_PARAMETER_H

#include <juce_audio_utils/juce_audio_utils.h>

#include <utility>
#include "../ModulationMatrix.h"
#include "Params.h"

using namespace Params;


template<typename T>
class Parameter {
public:
    Parameter(juce::Identifier paramId, ModulationMatrix &matrix) :
            modulationMatrix(matrix), paramId(std::move(paramId)) {}

    T getValue() const {
        auto [baseValue, modValue] = modulationMatrix.getParamAndModulationValue(paramId);
        float result = baseValue + modValue;
        result = std::clamp(result, 0.0f, 1.0f);

        if constexpr (std::is_same_v<T, float>) {
            return result;
        } else if constexpr (std::is_same_v<T, bool>) {
            return result > 0.5f;
        } else if constexpr (std::is_integral_v<T>) {
            return static_cast<T>(std::round(result));
        } else if constexpr (std::is_enum_v<T>) {
            return static_cast<T>(static_cast<int>(result));
        } else {
            jassert(false && "Unsupported parameter type!");
            return T();
        }
    }


private:
    juce::Identifier paramId;
    ModulationMatrix &modulationMatrix;
};

#endif //COINCIDENCE_PARAMETER_H
