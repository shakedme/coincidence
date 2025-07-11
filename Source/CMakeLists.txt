project(Coincidence VERSION 0.0.1)

set (BaseTargetName Coincidence)

juce_add_plugin("${BaseTargetName}"
        # VERSION ...
        # ICON_BIG ...
        # ICON_SMALL ...
        COMPANY_NAME "Shaduki"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT TRUE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE
        PLUGIN_MANUFACTURER_CODE Juce
        PLUGIN_CODE COIN
        FORMATS VST3 Standalone
        PRODUCT_NAME "Coincidence")

# Disable unity builds for the standalone target to prevent header inclusion issues
set_target_properties(${BaseTargetName}_Standalone PROPERTIES UNITY_BUILD OFF)

target_compile_features(${BaseTargetName} PRIVATE cxx_std_20)

target_sources(${BaseTargetName} PRIVATE
        Gui/LookAndFeel.cpp
        Gui/PluginEditor.cpp

        Gui/Sections/BaseSection.cpp
        Gui/Sections/GrooveSection.cpp
        Gui/Sections/EnvelopeSection.cpp
        Gui/Sections/PitchSection.cpp
        Gui/Sections/SampleSection.cpp
        Gui/Sections/EffectsSection.cpp

        Gui/Components/DirectionButton.h
        Gui/Components/DirectionSelector.h
        Gui/Components/Sample/SampleDetail.h
        Gui/Components/Sample/SampleList.cpp
        Gui/Components/Sample/GroupListView.h
        Gui/Components/Sample/SampleRow.cpp
        Gui/Components/Toggle.h
        Gui/Components/Envelope/EnvelopeComponent.cpp
        Gui/Components/HeaderComponent.h
        Gui/Components/WaveformComponent.cpp
        Gui/Components/KnobComponent.cpp
        Gui/Components/Envelope/EnvelopePresetGenerator.cpp
        Gui/Components/Envelope/EnvelopePointManager.cpp
        Gui/Components/Envelope/EnvelopeRenderer.cpp
        Gui/Components/Envelope/EnvelopeShapeButton.h
        Gui/Components/Envelope/EnvelopeParameterMapper.cpp
        Gui/Components/Envelope/EnvelopePoint.h

        Shared/Models.h
        Shared/Parameters/ParameterLoader.cpp
        Shared/Parameters/Params.h
        Shared/Parameters/Parameter.h
        Shared/Parameters/StructParameter.h
        Shared/TimingManager.cpp
        Shared/ModulationMatrix.cpp

        Audio/PluginProcessor.cpp
        Audio/Sampler/SampleManager.cpp
        Audio/Sampler/SamplerVoice.cpp
        Audio/Sampler/SamplerVoiceState.cpp
        Audio/Sampler/SamplerSound.cpp
        Audio/Sampler/OnsetDetector.cpp
        Audio/Midi/ScaleManager.cpp
        Audio/Midi/NoteGenerator.cpp
        Audio/FileLogger.cpp
        Audio/Effects/FxEngine.cpp
        Audio/Effects/Stutter.cpp
        Audio/Effects/Reverb.cpp
        Audio/Effects/Delay.cpp
        Audio/Effects/Gain.cpp
        Audio/Effects/BaseEffect.cpp
        Audio/Effects/Compression.cpp
        Audio/Effects/Pan.cpp
        Audio/Effects/Flanger.cpp
        Audio/Effects/Phaser.cpp
        Audio/Util/AudioBufferQueue.h)

target_compile_definitions(${BaseTargetName}
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)


juce_add_binary_data(CoincidenceAssets
        SOURCES
        ${CMAKE_SOURCE_DIR}/Assets/Icons/pencil.svg
        ${CMAKE_SOURCE_DIR}/Assets/Icons/three-lines.svg
        ${CMAKE_SOURCE_DIR}/Assets/Icons/delete.svg
        ${CMAKE_SOURCE_DIR}/Assets/audio-parameters.json
        )

target_link_libraries(${BaseTargetName} PRIVATE
        CoincidenceAssets
        juce_audio_utils
        juce_dsp
        juce_recommended_config_flags
        juce_recommended_lto_flags
        juce_recommended_warning_flags
        melatonin_inspector
        )

add_custom_target(Clean
        COMMAND ${CMAKE_COMMAND} --build . --target clean
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )