cmake_minimum_required(VERSION 3.15)

# CMake 3.28+ yells about add_custom_command(TARGET ... DEPENDS) via CMP0175.
# JUCE still leans on that pattern, and we're not here to rewrite upstream.
# Pinning the policy to OLD keeps configure output focused on the fun parts
# (the DSP) instead of scolding us about someone else's arguments.
if(POLICY CMP0175)
  cmake_policy(SET CMP0175 OLD)
endif()
project(HorizonPlugin VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 7.0.9
)
FetchContent_MakeAvailable(juce)

juce_add_plugin(HorizonJuce
  COMPANY_NAME "Horizon"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  COPY_PLUGIN_AFTER_BUILD TRUE
  PLUGIN_MANUFACTURER_CODE Horz
  PLUGIN_CODE HzJc
  FORMATS VST3 Standalone
  PRODUCT_NAME "Horizon"
)

target_sources(HorizonJuce PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginProcessor.h
  Source/PluginEditor.cpp
  Source/PluginEditor.h
  ../src/host/HostHorizonProcessor.cpp
  ../src/host/HostHorizonProcessor.h
  ../src/AirEQ.cpp
  ../src/DynWidth.cpp
  ../src/LimiterLookahead.cpp
  ../src/MSMatrix.cpp
  ../src/ParamSmoother.cpp
  ../src/SoftSaturation.cpp
  ../src/TiltEQ.cpp
  ../src/TransientDetector.cpp
)

target_include_directories(HorizonJuce PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Source
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/host
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs
)

target_compile_definitions(HorizonJuce
  PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(HorizonJuce PRIVATE juce::juce_audio_utils juce::juce_dsp)
