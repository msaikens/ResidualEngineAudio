cmake_minimum_required(VERSION 3.20)

project(ResidualVoiceEngine LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(RV_FETCH_OPUS "Fetch libopus with CMake FetchContent" ON)
option(RV_BUILD_UDP_SHIM "Build optional Win32 UDP shim" OFF)
option(RV_BUILD_EXAMPLES "Build examples" OFF)

# ----------------------------
# Opus
# ----------------------------
if (RV_FETCH_OPUS)
    include(FetchContent)

    FetchContent_Declare(
        opus
        GIT_REPOSITORY https://github.com/xiph/opus.git
        GIT_TAG v1.5.2
    )

    FetchContent_MakeAvailable(opus)

    if (TARGET opus)
        set_target_properties(opus PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()
else()
    find_package(Opus REQUIRED)
endif()

# ----------------------------
# Core voice library
# ----------------------------
add_library(residual_voice SHARED
    src/voice.c
    src/rv_opus.c
    src/rv_opus_jitter.c
    src/rv_netproto.c
    src/rv_shim_transport.c
)

target_compile_definitions(residual_voice PRIVATE
    RV_VOICE_BUILD_DLL=1
    _CRT_SECURE_NO_WARNINGS=1
)

target_include_directories(residual_voice
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (TARGET opus)
    target_link_libraries(residual_voice PRIVATE opus)
else()
    target_link_libraries(residual_voice PRIVATE Opus::opus)
endif()

set_target_properties(residual_voice PROPERTIES
    OUTPUT_NAME "residual_voice"
)

if (CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(residual_voice PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -fvisibility=hidden
    )
elseif (MSVC)
    target_compile_options(residual_voice PRIVATE
        /W4
        /experimental:c11atomics
    )
endif()

# ----------------------------
# Optional UDP shim
# Keep this OFF by default for Unity.
# Unity/game code should usually own transport.
# ----------------------------
if (RV_BUILD_UDP_SHIM)
    if (WIN32)
        target_sources(residual_voice PRIVATE
            src/rv_shim_udp.c
            src/rv_udp_win32.c
        )

        target_link_libraries(residual_voice PRIVATE ws2_32)
    else()
        message(FATAL_ERROR "RV_BUILD_UDP_SHIM currently only supports Win32.")
    endif()
endif()

# ----------------------------
# Optional examples
# ----------------------------
if (RV_BUILD_EXAMPLES)
    add_executable(voice_demo
        examples/voice_demo.c
    )

    target_link_libraries(voice_demo PRIVATE residual_voice)
endif()
# ----------------------------
# Unity package output
# ----------------------------
option(RV_COPY_UNITY_PLUGIN "Copy built native library into Unity package folder" ON)

if (RV_COPY_UNITY_PLUGIN AND WIN32)
    add_custom_command(TARGET residual_voice POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/unity/com.residual.voice/Runtime/Plugins/Windows/x86_64"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:residual_voice>"
            "${CMAKE_CURRENT_SOURCE_DIR}/unity/com.residual.voice/Runtime/Plugins/Windows/x86_64/$<TARGET_FILE_NAME:residual_voice>"
        COMMENT "Copying residual_voice native plugin into Unity package"
    )
endif()