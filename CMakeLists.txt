cmake_minimum_required(VERSION 3.16)
project(horizon_dsp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(HORIZON_DSP_SOURCES
  src/AirEQ.cpp
  src/DynWidth.cpp
  src/LimiterLookahead.cpp
  src/MSMatrix.cpp
  src/ParamSmoother.cpp
  src/SoftSaturation.cpp
  src/TiltEQ.cpp
  src/TransientDetector.cpp
  src/host/HostHorizonProcessor.cpp
)

add_library(horizon_dsp ${HORIZON_DSP_SOURCES})
add_library(horizon_dsp::horizon_dsp ALIAS horizon_dsp)

target_include_directories(horizon_dsp
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/host>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(horizon_dsp PUBLIC cxx_std_17)

include(GNUInstallDirs)
install(TARGETS horizon_dsp
  EXPORT horizon_dspTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY src/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/horizon
  FILES_MATCHING PATTERN "*.h"
)

install(EXPORT horizon_dspTargets
  FILE horizon_dspTargets.cmake
  NAMESPACE horizon_dsp::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/horizon_dsp
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/horizon_dspConfigVersion.cmake
  VERSION "0.1.0"
  COMPATIBILITY SameMajorVersion
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/docs/cmake/horizon_dspConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/horizon_dspConfig.cmake
  COPYONLY
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/horizon_dspConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/horizon_dspConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/horizon_dsp
)

option(HORIZON_BUILD_CLI "Build the host CLI renderer" ON)
option(HORIZON_BUILD_PORTAUDIO_DEMO "Build the PortAudio live demo" OFF)

if(HORIZON_BUILD_CLI)
  find_package(SndFile QUIET)
  if(NOT TARGET SndFile::sndfile AND TARGET sndfile)
    add_library(SndFile::sndfile ALIAS sndfile)
  endif()
  if(SndFile_FOUND)
    add_executable(horizon_cli apps/horizon_cli/horizon_cli.cpp)
    target_include_directories(horizon_cli PRIVATE src src/host)
    target_link_libraries(horizon_cli PRIVATE horizon_dsp SndFile::sndfile)
  else()
    message(STATUS "libsndfile not found; horizon_cli target skipped. Install libsndfile-dev to enable it.")
  endif()
endif()

if(HORIZON_BUILD_PORTAUDIO_DEMO)
  find_package(PortAudio QUIET)
  if(NOT TARGET PortAudio::PortAudio AND TARGET PortAudio)
    add_library(PortAudio::PortAudio ALIAS PortAudio)
  elseif(NOT TARGET PortAudio::PortAudio AND TARGET portaudio)
    add_library(PortAudio::PortAudio ALIAS portaudio)
  endif()
  if(PortAudio_FOUND)
    add_executable(horizon_portaudio apps/portaudio_demo/horizon_portaudio.cpp)
    target_include_directories(horizon_portaudio PRIVATE src src/host)
    target_link_libraries(horizon_portaudio PRIVATE horizon_dsp PortAudio::PortAudio)
  else()
    message(STATUS "PortAudio not found; horizon_portaudio target skipped. Install portaudio-dev to enable it.")
  endif()
endif()

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  # Keep editors happy without hunting through build trees. Mirrors the PlatformIO lint shim flow
  # but with the host-friendly flags emitted by CMake.
  add_custom_target(sync_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_BINARY_DIR}/compile_commands.json
      ${CMAKE_SOURCE_DIR}/compile_commands.json
    BYPRODUCTS ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Syncing compile_commands.json to repo root for clangd/IntelliSense")
endif()
