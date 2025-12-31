cmake_minimum_required(VERSION 3.20)
project(ResidualVoiceEngine C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# ----------------------------
# Opus (FetchContent)
# ----------------------------
FetchContent_Declare(
  opus
  GIT_REPOSITORY https://github.com/xiph/opus.git
  GIT_TAG        v1.5.2
)
FetchContent_MakeAvailable(opus)

# Ensure we can link it into a shared lib cleanly
set_target_properties(opus PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ----------------------------
# Core + UDP shim as OBJECT libs
# (so residual_voice.dll compiles directly from these objects)
# ----------------------------
add_library(residual_voice_core_obj OBJECT
  src/voice.c
  src/rv_opus.c
  src/rv_opus_jitter.c
  src/rv_netproto.c
  src/rv_shim_transport.c
)
target_include_directories(residual_voice_core_obj
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(residual_voice_core_obj PRIVATE opus)

add_library(residual_voice_udp_obj OBJECT
  src/rv_shim_udp.c
  src/rv_udp_win32.c
)
target_include_directories(residual_voice_udp_obj
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if (WIN32)
  target_link_libraries(residual_voice_udp_obj PRIVATE ws2_32)
endif()

# ----------------------------
# The one DLL/SO users consume
# ----------------------------
add_library(residual_voice SHARED
  $<TARGET_OBJECTS:residual_voice_core_obj>
  $<TARGET_OBJECTS:residual_voice_udp_obj>
)

target_compile_definitions(residual_voice PRIVATE RV_VOICE_BUILD_DLL=1)
target_include_directories(residual_voice PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(residual_voice PROPERTIES
  OUTPUT_NAME "residual_voice"
)
