#include "ParameterLoader.h"
#include "Models.h"
#include "BinaryData.h"

namespace AppState {

    juce::var ParameterLoader::loadParametersJson() {
        auto jsonString = juce::String(BinaryData::audioparameters_json, BinaryData::audioparameters_jsonSize);
        return juce::JSON::parse(jsonString);
    }

    void ParameterLoader::addParameterFromJson(juce::AudioProcessorValueTreeState::ParameterLayout &layout,
                                               const juce::var &paramData) {
        if (paramData.hasProperty("type") && paramData["type"].toString() == "dynamic") {
            createDynamicParameters(layout, paramData);
            return;
        }
        if (auto param = createParameterFromJson(paramData)) {
            layout.add(std::move(param));
        }
    }

    std::unique_ptr<juce::RangedAudioParameter> ParameterLoader::createParameterFromJson(const juce::var &paramData) {
        if (!paramData.hasProperty("id") || !paramData.hasProperty("type")) {
            juce::Logger::writeToLog("Parameter missing required properties");
            return nullptr;
        }

        auto id = paramData["id"].toString();
        auto name = paramData.hasProperty("name") ? paramData["name"].toString() : id;
        auto type = paramData["type"].toString();

        if (type == "int") {
            auto min = static_cast<int>(paramData["min"]);
            auto max = static_cast<int>(paramData["max"]);
            auto defaultValue = static_cast<int>(paramData["default"]);

            return std::make_unique<juce::AudioParameterInt>(
                    id, name, min, max, defaultValue);
        } else if (type == "float") {
            auto min = static_cast<float>(paramData["min"]);
            auto max = static_cast<float>(paramData["max"]);
            auto defaultValue = static_cast<float>(paramData["default"]);

            return std::make_unique<juce::AudioParameterFloat>(
                    id, name, min, max, defaultValue);
        } else if (type == "bool") {
            auto defaultValue = static_cast<bool>(paramData["default"]);

            return std::make_unique<juce::AudioParameterBool>(
                    id, name, defaultValue);
        } else if (type == "choice") {
            juce::StringArray choices;
            auto options = paramData["options"];

            for (int i = 0; i < options.size(); ++i) {
                choices.add(options[i].toString());
            }

            auto defaultValue = static_cast<int>(paramData["default"]);

            return std::make_unique<juce::AudioParameterChoice>(
                    id, name, choices, defaultValue);
        }

        juce::Logger::writeToLog("Unsupported parameter type: " + type);
        return nullptr;
    }

    void ParameterLoader::createDynamicParameters(juce::AudioProcessorValueTreeState::ParameterLayout &layout,
                                                  const juce::var &paramData) {
        // This handles special cases like the rate parameters which are defined dynamically
        if (paramData["id"] == "rates") {
            // Access the array of rate base names from the Models namespace
            for (auto &rateName: Models::rateBaseNames) {
                auto min = static_cast<float>(paramData["min"]);
                auto max = static_cast<float>(paramData["max"]);
                auto defaultValue = static_cast<float>(paramData["default"]);
                auto namePattern = paramData["names"].toString();
                auto type = paramData["parameter_type"].toString();

                // Replace $NAME with the actual rate name
                juce::String displayName = namePattern.replace("$NAME", rateName);

                if (type == "int") {
                    layout.add(std::make_unique<juce::AudioParameterInt>(
                            rateName, displayName, static_cast<int>(min), static_cast<int>(max),
                            static_cast<int>(defaultValue)));
                } else if (type == "float") {
                    layout.add(std::make_unique<juce::AudioParameterFloat>(
                            rateName, displayName, min, max, defaultValue));
                }
            }
        }
    }

} // namespace AppState 