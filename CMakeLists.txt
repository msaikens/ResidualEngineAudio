cmake_minimum_required(VERSION 3.20)
project(ResidualVoiceEngine C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Library
add_library(residual_voice STATIC
    src/voice.c
    src/rv_opus.c
    src/rv_opus_jitter.c
    src/rv_netproto.c
    src/rv_udp_win32.c
)

target_include_directories(residual_voice
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Warnings
if (MSVC)
  target_compile_options(residual_voice PRIVATE /W4)
else()
  target_compile_options(residual_voice PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Winsock (Windows)
target_link_libraries(residual_voice PRIVATE ws2_32)

# Opus (vcpkg)
find_package(Opus CONFIG QUIET)
if (Opus_FOUND)
  target_link_libraries(residual_voice PRIVATE Opus::opus)
else()
  find_package(unofficial-opus CONFIG REQUIRED)
  target_link_libraries(residual_voice PRIVATE unofficial::opus::opus)
endif()

# Example client app
add_executable(voice_demo examples/voice_demo.c)
target_link_libraries(voice_demo PRIVATE residual_voice)
target_include_directories(voice_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Relay server exe (uses the same udp + netproto files)
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
