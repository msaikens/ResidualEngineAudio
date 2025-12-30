cmake_minimum_required(VERSION 3.20)
project(ResidualVoiceEngine C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ----------------------------
# Core (freestanding)
# ----------------------------
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

# Warnings
if (MSVC)
  target_compile_options(residual_voice_core PRIVATE /W4)
else()
  target_compile_options(residual_voice_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Opus (vcpkg)
find_package(Opus CONFIG QUIET)
if (Opus_FOUND)
  target_link_libraries(residual_voice_core PRIVATE Opus::opus)
else()
  find_package(unofficial-opus CONFIG REQUIRED)
  target_link_libraries(residual_voice_core PRIVATE unofficial::opus::opus)
endif()

# ----------------------------
# UDP shim (platform module)
# ----------------------------
add_library(residual_voice_shim_udp STATIC
    src/rv_shim_udp.c
    src/rv_udp_win32.c
)

target_include_directories(residual_voice_shim_udp
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Warnings
if (MSVC)
  target_compile_options(residual_voice_shim_udp PRIVATE /W4)
else()
  target_compile_options(residual_voice_shim_udp PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Winsock (Windows)
target_link_libraries(residual_voice_shim_udp PRIVATE ws2_32)

# Make sure UDP shim can see core headers/types
target_link_libraries(residual_voice_shim_udp PUBLIC residual_voice_core)

# ----------------------------
# Back-compat aggregate target (optional)
# Keeps "residual_voice" name for your existing users.
# ----------------------------
add_library(residual_voice INTERFACE)
target_link_libraries(residual_voice INTERFACE residual_voice_core residual_voice_shim_udp)

# ----------------------------
# Example client app
# ----------------------------
add_executable(voice_demo examples/voice_demo.c)
target_link_libraries(voice_demo PRIVATE residual_voice)  # or residual_voice_core + residual_voice_shim_udp
target_include_directories(voice_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ----------------------------
# Relay server exe
# (no opus, no voice core needed)
# ----------------------------
add_executable(residual_relay
    src/rv_relay_main.c
    src/rv_relay.c
    src/rv_netproto.c
    src/rv_udp_win32.c
)

target_include_directories(residual_relay PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(residual_relay PRIVATE ws2_32)
