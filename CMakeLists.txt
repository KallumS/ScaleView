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
option(SCALEVIEW_BUILD_CLAP   "Also build the CLAP (downloads clap-juce-extensions)" ON)

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
    NEEDS_MIDI_INPUT          TRUE       # it reads MIDI to name the chord being played
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE      # set ON to install into your plugin folder on build
    VST3_CATEGORIES           "Analyzer" "Tools"
    # AU_MAIN_TYPE is deliberately not set. JUCE only picks a type when the
    # property is unset, and with NEEDS_MIDI_INPUT it picks MusicEffect, which
    # is what makes Logic send MIDI to an effect. Forcing Effect here would
    # silently stop the chord display working in Logic.
    )

target_sources(ScaleView PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(ScaleView PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

#==============================================================================
# A second test binary for the MIDI and state handling, which needs JUCE.
#==============================================================================
if(SCALEVIEW_BUILD_TESTS)
    juce_add_console_app(ScaleViewProcessorTests PRODUCT_NAME "ScaleViewProcessorTests")

    target_sources(ScaleViewProcessorTests PRIVATE
        Tests/TestProcessorMidi.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

    target_compile_definitions(ScaleViewProcessorTests PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1
        JucePlugin_Name="ScaleView"
        JucePlugin_IsSynth=0
        JucePlugin_WantsMidiInput=1
        JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0)

    target_link_libraries(ScaleViewProcessorTests
        PRIVATE
            ScaleViewModel
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    add_test(NAME ProcessorMidi COMMAND ScaleViewProcessorTests)
endif()

target_link_libraries(ScaleView
    PRIVATE
        ScaleViewModel
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

#==============================================================================
# CLAP. JUCE does not build this format itself, so the plugin is wrapped by
# clap-juce-extensions, which adds a CLAP target beside the VST3 and AU from
# the same sources. Turn it off with -DSCALEVIEW_BUILD_CLAP=OFF.
#==============================================================================
if(SCALEVIEW_BUILD_CLAP)
    # Pinned to a commit rather than the 0.26.0 release: that release predates
    # JUCE 8.0.12 moving juce_LegacyAudioParameter into the new headless
    # module, which the wrapper includes. This commit handles both layouts.
    FetchContent_Declare(clap-juce-extensions
        GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
        GIT_TAG        9fbefae3d9c3d130aafb558c1ec15427a4bd24be)

    FetchContent_MakeAvailable(clap-juce-extensions)

    clap_juce_extensions_plugin(TARGET ScaleView
        CLAP_ID       "com.kallums.scaleview"
        CLAP_FEATURES audio-effect analyzer utility)
endif()
