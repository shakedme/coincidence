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
        FORMATS AU VST3 Standalone
        PRODUCT_NAME "Coincidence")

target_sources(${BaseTargetName} PRIVATE
        Gui/LookAndFeel.cpp
        Gui/PluginEditor.cpp
        Gui/Sections/BaseSection.cpp
        Gui/Sections/EffectsSection.cpp
        Gui/Sections/GrooveSection.cpp
        Gui/Sections/EnvelopeSection.cpp
        Gui/Sections/PitchSection.cpp
        Gui/Sections/SampleSection.cpp
        Gui/Components/DirectionButton.h
        Gui/Components/DirectionSelector.h
        Gui/Components/SampleDetail.h
        Gui/Components/SampleList.cpp
        Gui/Components/GroupListView.h
        Gui/Components/SampleRow.cpp
        Gui/Components/Toggle.h
        Gui/Components/EnvelopeComponent.cpp
        Gui/Components/HeaderComponent.h
        Audio/Params.cpp
        Audio/PluginProcessor.cpp
        Audio/Sampler/SampleManager.cpp
        Audio/Sampler/Sampler.cpp
        Audio/Sampler/OnsetDetector.cpp
        Audio/Shared/TimingManager.cpp
        Audio/Midi/ScaleManager.cpp
        Audio/Midi/NoteGenerator.cpp
        Audio/CoincidenceAudioProcessor.cpp
        Audio/FileLogger.cpp
        Audio/Effects/FxEngine.cpp
        Audio/Effects/Stutter.cpp
        Audio/Effects/Reverb.cpp
        Audio/Effects/Delay.cpp
        Audio/Effects/BaseEffect.cpp
        Audio/Util/AudioBufferQueue.h
        Audio/Util/AudioOscilloscope.cpp
        )

target_compile_definitions(${BaseTargetName}
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)


juce_add_binary_data(CoincidenceAssets
        SOURCES
        ../Assets/Icons/pencil.svg
        ../Assets/Icons/three-lines.svg
        ../Assets/Icons/delete.svg
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