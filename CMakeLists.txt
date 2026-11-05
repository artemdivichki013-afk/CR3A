cmake_minimum_required(VERSION 3.22)
project(CR3A VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(CR3A_BUILD_TESTS "Build DSP test and UI snapshot tools" OFF)
option(CR3A_MINIMAL_LINUX_DEPS "Linux only: do not require ALSA/Xrandr/Xinerama/Xcursor" OFF)
set(CR3A_JUCE_PATH "" CACHE PATH "Path to an existing JUCE checkout (optional, otherwise it is downloaded)")

# ---------------------------------------------------------------- JUCE
if(CR3A_JUCE_PATH AND EXISTS "${CR3A_JUCE_PATH}/CMakeLists.txt")
    add_subdirectory("${CR3A_JUCE_PATH}" juce EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.9
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable(juce)
endif()

# ---------------------------------------------------------------- sources
set(CR3A_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/Parameters.h
    Source/Presets.h
    Source/DSP/Utils.h
    Source/DSP/Formant.h
    Source/DSP/Doubler.h
    Source/DSP/VocalEQ.h
    Source/DSP/Compressor.h
    Source/DSP/Saturator.h
    Source/DSP/OTT.h
    Source/DSP/StereoDelay.h
    Source/DSP/Reverb.h
    Source/DSP/Width.h
    Source/DSP/Limiter.h
    Source/UI/Theme.h
    Source/UI/CR3ALookAndFeel.h
    Source/UI/CR3ALookAndFeel.cpp
    Source/UI/Widgets.h
    Source/UI/Display.h
    Source/PluginEditor.h
    Source/PluginEditor.cpp)

juce_add_binary_data(CR3A_Assets SOURCES Assets/logo.png)
set_target_properties(CR3A_Assets PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(CR3A_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_JACK=0)

if(CR3A_MINIMAL_LINUX_DEPS AND UNIX AND NOT APPLE)
    list(APPEND CR3A_DEFS JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_ALSA=0)
endif()

set(CR3A_FORMATS VST3)
if(WIN32 OR APPLE)
    list(APPEND CR3A_FORMATS Standalone)
endif()
if(APPLE)
    list(APPEND CR3A_FORMATS AU)
endif()

# ---------------------------------------------------------------- plugin
juce_add_plugin(CR3A
    COMPANY_NAME                "CR3A"
    PLUGIN_MANUFACTURER_CODE    Cr3a
    PLUGIN_CODE                 Cr3A
    PRODUCT_NAME                "CR3A"
    DESCRIPTION                 "CR3A vocal chain"
    FORMATS                     ${CR3A_FORMATS}
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Fx
    AU_MAIN_TYPE                kAudioUnitType_Effect)

juce_generate_juce_header(CR3A)

target_sources(CR3A PRIVATE ${CR3A_SOURCES})
target_include_directories(CR3A PRIVATE Source)
target_compile_definitions(CR3A PUBLIC ${CR3A_DEFS})
target_link_libraries(CR3A
    PRIVATE
        CR3A_Assets
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

if(MSVC)
    target_compile_options(CR3A PRIVATE /bigobj /utf-8)
endif()

# ---------------------------------------------------------------- tests (dev only)
if(CR3A_BUILD_TESTS)
    juce_add_console_app(CR3A_Test PRODUCT_NAME "CR3A_Test")
    target_sources(CR3A_Test PRIVATE ${CR3A_SOURCES} Tests/dsp_test.cpp)
    target_include_directories(CR3A_Test PRIVATE Source)
    target_compile_definitions(CR3A_Test PRIVATE ${CR3A_DEFS} JUCE_PLUGINHOST_AU=0 JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(CR3A_Test PRIVATE
        CR3A_Assets
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_recommended_config_flags)

    juce_add_console_app(CR3A_Snap PRODUCT_NAME "CR3A_Snap")
    target_sources(CR3A_Snap PRIVATE ${CR3A_SOURCES} Tests/snapshot.cpp)
    target_include_directories(CR3A_Snap PRIVATE Source)
    target_compile_definitions(CR3A_Snap PRIVATE ${CR3A_DEFS} JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(CR3A_Snap PRIVATE
        CR3A_Assets
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_recommended_config_flags)
endif()
