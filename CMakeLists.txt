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

# Native GXM debug-view shaders.
if(DEFINED ENV{PSP2CGC} AND EXISTS "$ENV{PSP2CGC}")
 set(PSP2CGC_EXECUTABLE "$ENV{PSP2CGC}")
else()
 find_program(PSP2CGC_EXECUTABLE NAMES psp2cgc psp2cgc.exe)
endif()
find_program(VITA_OBJCOPY NAMES arm-vita-eabi-objcopy arm-vita-eabi-objcopy.exe)

if(NOT PSP2CGC_EXECUTABLE)
 message(FATAL_ERROR "psp2cgc was not found. Set PSP2CGC to the full path of psp2cgc.exe or add it to PATH.")
endif()
if(NOT VITA_OBJCOPY)
 message(FATAL_ERROR "arm-vita-eabi-objcopy was not found in PATH.")
endif()

add_custom_command(
 OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lagi_color_v.gxp"
 COMMAND "${PSP2CGC_EXECUTABLE}" -profile sce_vp_psp2
         "${CMAKE_CURRENT_SOURCE_DIR}/shaders/lagi_color_v.cg"
         -Wperf -fastprecision -O3
         -o "${CMAKE_CURRENT_BINARY_DIR}/lagi_color_v.gxp"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/lagi_color_v.cg"
 COMMENT "Compiling GXM vertex shader"
)
add_custom_command(
 OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lagi_color_f.gxp"
 COMMAND "${PSP2CGC_EXECUTABLE}" -profile sce_fp_psp2
         "${CMAKE_CURRENT_SOURCE_DIR}/shaders/lagi_color_f.cg"
         -Wperf -fastprecision -O3
         -o "${CMAKE_CURRENT_BINARY_DIR}/lagi_color_f.gxp"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/lagi_color_f.cg"
 COMMENT "Compiling GXM fragment shader"
)
add_custom_command(
 OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lagi_color_v_gxp.o"
 COMMAND "${VITA_OBJCOPY}" --input binary --output elf32-littlearm
         --binary-architecture arm lagi_color_v.gxp lagi_color_v_gxp.o
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/lagi_color_v.gxp"
 COMMENT "Embedding GXM vertex shader"
)
add_custom_command(
 OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lagi_color_f_gxp.o"
 COMMAND "${VITA_OBJCOPY}" --input binary --output elf32-littlearm
         --binary-architecture arm lagi_color_f.gxp lagi_color_f_gxp.o
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/lagi_color_f.gxp"
 COMMENT "Embedding GXM fragment shader"
)
set(LAGI_GXM_SHADER_OBJECTS
 "${CMAKE_CURRENT_BINARY_DIR}/lagi_color_v_gxp.o"
 "${CMAKE_CURRENT_BINARY_DIR}/lagi_color_f_gxp.o"
)
set_source_files_properties(${LAGI_GXM_SHADER_OBJECTS} PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)

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
 ${LAGI_GXM_SHADER_OBJECTS}
)
target_include_directories(lagi PRIVATE include
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel"
)
target_compile_definitions(lagi PRIVATE LAGI_PLATFORM_VITA=1 LAGI_NULL_RENDERER=1 LAGI_NULL_AUDIO=1 USE_NULL_RENDERER=1 SHIPPING_BUILD=1)
target_link_libraries(lagi m SceCtrl_stub SceDisplay_stub SceGxm_stub SceKernelThreadMgr_stub SceSysmem_stub SceSysmodule_stub)

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
