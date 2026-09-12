cmake_minimum_required(VERSION 3.22)

project(ScaleView VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    # Covers every Mac still receiving updates, Intel and Apple silicon.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build a universal binary")
endif()

option(SCALEVIEW_BUILD_PLUGIN "Build the VST3/AU plugin (downloads JUCE)" ON)
option(SCALEVIEW_BUILD_TESTS  "Build the scale model tests (no JUCE needed)" ON)

#==============================================================================
# The musical core: header only, no JUCE, no host.
#==============================================================================
add_library(ScaleViewModel INTERFACE)
target_include_directories(ScaleViewModel INTERFACE Source)
target_compile_features(ScaleViewModel INTERFACE cxx_std_17)

if(SCALEVIEW_BUILD_TESTS)
    enable_testing()
    add_executable(ScaleViewTests Tests/TestScaleModel.cpp)
    target_link_libraries(ScaleViewTests PRIVATE ScaleViewModel)
    add_test(NAME ScaleModel COMMAND ScaleViewTests)
endif()

if(NOT SCALEVIEW_BUILD_PLUGIN)
    return()
endif()

#==============================================================================
# JUCE, pinned. Bump the tag to move version; 9.x works the same way.
#==============================================================================
include(FetchContent)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.15
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(JUCE)

#==============================================================================
# The plugin. On macOS this produces both the VST3 and the AU; elsewhere the
# AU is skipped automatically, as Apple only allows it to be built on a Mac.
#==============================================================================
juce_add_plugin(ScaleView
    COMPANY_NAME              "kallums"
    BUNDLE_ID                 "com.kallums.scaleview"
    PLUGIN_MANUFACTURER_CODE  Klms
    PLUGIN_CODE               Scvw
    FORMATS                   VST3 AU Standalone
    PRODUCT_NAME              "ScaleView"
    DESCRIPTION               "Key signature and scale reference"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE      # set ON to install into your plugin folder on build
    VST3_CATEGORIES           "Analyzer" "Tools"
    AU_MAIN_TYPE              "kAudioUnitType_Effect")

target_sources(ScaleView PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(ScaleView PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(ScaleView
    PRIVATE
        ScaleViewModel
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
