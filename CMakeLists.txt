cmake_minimum_required(VERSION 3.22)

project(VOSK VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE
# Uses a local JUCE checkout if one is found (fast local builds), otherwise
# fetches JUCE 8.0.6 from GitHub so the project builds anywhere.
# ---------------------------------------------------------------------------
set(JUCE_DIR "$ENV{JUCE_DIR}" CACHE PATH "Optional path to a local JUCE source tree")
if(NOT JUCE_DIR AND EXISTS "E:/JUCE/CMakeLists.txt")
    set(JUCE_DIR "E:/JUCE")
endif()

if(JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(STATUS "VOSK: using local JUCE at ${JUCE_DIR}")
    add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    message(STATUS "VOSK: fetching JUCE 8.0.6 from GitHub")
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.6
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# Auto-copy the built VST3 into a local plugins folder only when it exists
# (i.e. on the author's machine). Elsewhere JUCE copies to the OS default.
set(VOSK_LOCAL_COPY "")
if(WIN32 AND EXISTS "E:/Audio plugins")
    set(VOSK_LOCAL_COPY VST3_COPY_DIR "E:/Audio plugins")
endif()

# ---------------------------------------------------------------------------
# Plugin target
# Synth: accepts MIDI, no audio input (bus layout set in the processor ctor).
# ---------------------------------------------------------------------------
juce_add_plugin(VOSK
    COMPANY_NAME              "Mantis Vex"
    BUNDLE_ID                 "com.mantisvex.vosk"
    PLUGIN_MANUFACTURER_CODE  Mvex
    PLUGIN_CODE               Vosk
    FORMATS                   VST3 Standalone
    PRODUCT_NAME              "VOSK"
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   TRUE
    ${VOSK_LOCAL_COPY}
    VST3_CATEGORIES           Instrument Synth)

# Embedded resources (the UI font, so it looks identical on every machine).
# Rajdhani (SIL OFL 1.1) — see Resources/FONT-OFL.txt.
juce_add_binary_data(VoskData SOURCES
    Resources/VoskFontRegular.ttf
    Resources/VoskFontBold.ttf)

target_sources(VOSK PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/VoskVoice.cpp)

target_compile_definitions(VOSK PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(VOSK
    PRIVATE
        VoskData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
