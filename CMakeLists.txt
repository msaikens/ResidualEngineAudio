cmake_minimum_required(VERSION 3.20)
project(ResidualVoiceEngine C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# ------------------------------------------------------------
# Opus via FetchContent (builds Opus from source, static)
# ------------------------------------------------------------
# Opus upstream uses autotools/meson in their canonical repo;
# the easiest FetchContent flow on Windows is to use a CMake-wrapped
# Opus project. This keeps your users from needing Opus installed.
#
# Option A (recommended): use a known CMake wrapper repo.
# Example: xiph/opus "cmake support" is inconsistent historically,
# so we fetch a wrapper that exposes "opus" target.
#
# If you prefer vendoring instead of fetching, I’ll give you a
# third_party layout next.
FetchContent_Declare(
  opus_cmake
  GIT_REPOSITORY https://github.com/xiph/opus.git
  GIT_TAG        v1.5.2
)
FetchContent_MakeAvailable(opus_cmake)

# Ensure Opus builds as a static lib (best for single DLL)
set_target_properties(opus PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

# ------------------------------------------------------------
# Core (freestanding)
# ------------------------------------------------------------
add_library(residual_voice_core STATIC
  src/voice.c
  src/rv_opus.c
  src/rv_opus_jitter.c
  src/rv_netproto.c
  src/rv_shim_transport.c
)

target_include_directories(residual_voice_core
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (MSVC)
  target_compile_options(residual_voice_core PRIVATE /W4)
else()
  target_compile_options(residual_voice_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Link Opus into the core (static)
target_link_libraries(residual_voice_core PUBLIC opus)

# ------------------------------------------------------------
# UDP shim (platform module)
# ------------------------------------------------------------
add_library(residual_voice_shim_udp STATIC
  src/rv_shim_udp.c
  src/rv_udp_win32.c
)

target_include_directories(residual_voice_shim_udp
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (MSVC)
  target_compile_options(residual_voice_shim_udp PRIVATE /W4)
else()
  target_compile_options(residual_voice_shim_udp PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (WIN32)
  target_link_libraries(residual_voice_shim_udp PRIVATE ws2_32)
endif()

target_link_libraries(residual_voice_shim_udp PUBLIC residual_voice_core)

# ------------------------------------------------------------
# SHARED DLL / SO that users consume
# ------------------------------------------------------------
add_library(residual_voice SHARED
  src/rv_dll_stub.c
)

# This macro flips voice.h into dllexport during this target build
target_compile_definitions(residual_voice PRIVATE RV_VOICE_BUILD_DLL=1)

target_link_libraries(residual_voice PRIVATE
  residual_voice_core
  residual_voice_shim_udp
)

target_include_directories(residual_voice PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Nice output names:
# Windows: residual_voice.dll
# Linux:   libresidual_voice.so
set_target_properties(residual_voice PROPERTIES
  OUTPUT_NAME "residual_voice"
)

# ------------------------------------------------------------
# Example client app
# ------------------------------------------------------------
add_executable(voice_demo examples/voice_demo.c)
target_link_libraries(voice_demo PRIVATE residual_voice)
target_include_directories(voice_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ------------------------------------------------------------
# Relay server exe (no opus, no voice core needed)
# ------------------------------------------------------------
add_executable(residual_relay
  src/rv_relay_main.c
  src/rv_relay.c
  src/rv_netproto.c
  src/rv_udp_win32.c
)

target_include_directories(residual_relay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
if (WIN32)
  target_link_libraries(residual_relay PRIVATE ws2_32)
endif()
