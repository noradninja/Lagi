cmake_minimum_required(VERSION 3.10)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
 if(DEFINED ENV{VITASDK})
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "VitaSDK toolchain")
 else()
  message(FATAL_ERROR "VITASDK is not set.")
 endif()
endif()
project(Lagi LANGUAGES C CXX)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Lagi")
set(VITA_TITLEID "LAGI00001")
set(VITA_VERSION "00.01")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(LAGI_AZEL_PROBE "Compile an Azel source portability probe" ON)

add_executable(lagi
 src/main.cpp
 src/platform/vita/vita_platform.cpp
 src/platform/vita/vita_input.cpp
 src/platform/vita/vita_filesystem.cpp
 src/platform/vita/iso9660.cpp
 src/platform/vita/null_renderer.cpp
 src/platform/vita/null_audio.cpp
 src/integration/azel_runtime_smoke.cpp
 src/integration/azel_fixedpoint_vita.cpp
 src/integration/azel_heap_vita.cpp
 src/integration/azel_task_vita.cpp
 src/integration/azel_saturn_memory_vita.cpp
 src/integration/azel_common_overlay_vita.cpp
 src/integration/azel_sound_table_vita.cpp
 src/integration/azel_dragon_common_vita.cpp
 src/integration/azel_hotpoint_vita.cpp
)
target_include_directories(lagi PRIVATE include
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel"
)
target_compile_definitions(lagi PRIVATE LAGI_PLATFORM_VITA=1 LAGI_NULL_RENDERER=1 LAGI_NULL_AUDIO=1 USE_NULL_RENDERER=1 SHIPPING_BUILD=1)
target_link_libraries(lagi SceCtrl_stub SceDisplay_stub SceKernelThreadMgr_stub SceSysmem_stub SceSysmodule_stub)

if(LAGI_AZEL_PROBE)
 if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib")
  message(FATAL_ERROR "Azel submodule missing. Run: git submodule update --init --recursive")
 endif()
 add_library(lagi_azel_probe OBJECT src/integration/azel_probe.cpp)
 target_include_directories(lagi_azel_probe PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib"
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel"
 )
 target_compile_definitions(lagi_azel_probe PRIVATE USE_NULL_RENDERER=1 LAGI_PLATFORM_VITA=1)

 add_library(lagi_azel_fixedpoint OBJECT
  src/integration/azel_fixedpoint_vita.cpp
  src/integration/azel_heap_vita.cpp
  src/integration/azel_task_vita.cpp
 )
 target_include_directories(lagi_azel_fixedpoint PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib"
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel"
 )
 target_compile_definitions(lagi_azel_fixedpoint PRIVATE
  USE_NULL_RENDERER=1
  LAGI_PLATFORM_VITA=1
  SHIPPING_BUILD=1
 )
endif()

vita_create_self(lagi.self lagi UNSAFE)
vita_create_vpk(Lagi.vpk ${VITA_TITLEID} lagi.self VERSION ${VITA_VERSION} NAME ${VITA_APP_NAME})
