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
        auto value = modulationMatrix.getParamModulationValue(paramId);
        if constexpr (std::is_same_v<T, float>) {
            return percentToFloat(value);
        } else if constexpr (std::is_same_v<T, bool>) {
            return toBool(value);
        } else if constexpr (std::is_integral_v<T>) {
            return toInt(value);
        } else if constexpr (std::is_enum_v<T>) {
            return toEnum<T>(value);
        } else {
            jassert(false && "Unsupported parameter type!");
        }
    }


private:
    juce::Identifier paramId;
    ModulationMatrix &modulationMatrix;
};

#endif //COINCIDENCE_PARAMETER_H
