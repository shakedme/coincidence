project(DummySynth VERSION 0.0.1)

set (BaseTargetName Jammer)

juce_add_plugin("${BaseTargetName}"
        # VERSION ...                               # Set this if the plugin version is different to the project version
        # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
        # ICON_SMALL ...
        COMPANY_NAME "Shaduki"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT TRUE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE
        PLUGIN_MANUFACTURER_CODE Juce
        PLUGIN_CODE Jamm
        FORMATS AU VST3 Standalone
        PRODUCT_NAME "Jammer")

target_sources(${BaseTargetName} PRIVATE
        Gui/MidiGeneratorLookAndFeel.cpp
        Gui/MidiGeneratorEditor.cpp
        Gui/Sections/BaseSection.cpp
        Gui/Sections/GlitchSection.cpp
        Gui/Sections/GrooveSection.cpp
        Gui/Sections/PitchSection.cpp
        Gui/Sections/SampleSection.cpp
        Audio/MidiGeneratorParams.cpp
        Audio/MidiGeneratorProcessor.cpp
        Audio/SampleManager.cpp
        Audio/Sampler.cpp
        )

target_compile_definitions(${BaseTargetName}
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(${BaseTargetName} PRIVATE
        shared_plugin_helpers
        juce_audio_utils
        juce_recommended_config_flags
        juce_recommended_lto_flags
        juce_recommended_warning_flags)
