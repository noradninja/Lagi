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

set(VITA_APP_NAME "Lagi 0.030-alpha")
set(VITA_TITLEID "LAGI00001")
set(VITA_VERSION "00.30")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(LAGI_AZEL_PROBE "Compile an Azel source portability probe" ON)
option(LAGI_AZEL_RUINS_LINK_TRIM "Temporarily discard unreachable Azel code for Ruins bring-up" ON)

# GXM viewer shaders are compiled from readable Cg source using Sony's
# psp2cgc so color/debug and texture paths share the same compiler/toolchain.
#
# PSP2CGC may be set either to the compiler executable itself:
#   C:/Dev/sdk/host_tools/bin/psp2cgc.exe
# or to the directory containing it:
#   C:/Dev/sdk/host_tools/bin
set(PSP2CGC_EXECUTABLE "")

if(DEFINED ENV{PSP2CGC} AND NOT "$ENV{PSP2CGC}" STREQUAL "")
 set(_LAGI_PSP2CGC_ENV "$ENV{PSP2CGC}")

 if(EXISTS "${_LAGI_PSP2CGC_ENV}" AND NOT IS_DIRECTORY "${_LAGI_PSP2CGC_ENV}")
  set(PSP2CGC_EXECUTABLE "${_LAGI_PSP2CGC_ENV}")
 elseif(IS_DIRECTORY "${_LAGI_PSP2CGC_ENV}")
  find_program(PSP2CGC_EXECUTABLE
   NAMES psp2cgc psp2cgc.exe
   HINTS "${_LAGI_PSP2CGC_ENV}"
   NO_DEFAULT_PATH
  )
 endif()
endif()

if(NOT PSP2CGC_EXECUTABLE)
 find_program(PSP2CGC_EXECUTABLE
  NAMES psp2cgc psp2cgc.exe
  HINTS
   "$ENV{SCE_PSP2_SDK_DIR}/host_tools/bin"
   "E:/PSVITA/sdk/host_tools/bin"
 )
endif()

if(NOT PSP2CGC_EXECUTABLE)
 message(FATAL_ERROR
  "psp2cgc is required for Lagi's textured GXM shaders. "
  "Set PSP2CGC to either the compiler executable or its containing directory, "
  "or set SCE_PSP2_SDK_DIR so host_tools/bin can be searched.")
endif()

message(STATUS "Using psp2cgc: ${PSP2CGC_EXECUTABLE}")

set(LAGI_COLOR_V_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_color_v.gxp")
set(LAGI_COLOR_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_color_f.gxp")
set(LAGI_COLOR_V_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_color_v_gxp.o")
set(LAGI_COLOR_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_color_f_gxp.o")

set(LAGI_GOURAUD_DEBUG_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_gouraud_debug_f.gxp")
set(LAGI_GOURAUD_DEBUG_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_gouraud_debug_f_gxp.o")

set(LAGI_TEXTURED_LIT_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_lit_f.gxp")
set(LAGI_TEXTURED_LIT_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_lit_f_gxp.o")
set(LAGI_TEXTURED_LIT_OPAQUE_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_lit_opaque_f.gxp")
set(LAGI_TEXTURED_LIT_OPAQUE_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_lit_opaque_f_gxp.o")
set(LAGI_TEXTURED_LIT_HALF_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_lit_half_f.gxp")
set(LAGI_TEXTURED_LIT_HALF_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_lit_half_f_gxp.o")
set(LAGI_TEXTURED_LIT_NEWTON_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_lit_newton_f.gxp")
set(LAGI_TEXTURED_LIT_NEWTON_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_lit_newton_f_gxp.o")
set(LAGI_TEXTURED_PAYLOAD_PROBE_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_payload_probe_f.gxp")
set(LAGI_TEXTURED_PAYLOAD_PROBE_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_payload_probe_f_gxp.o")
set(LAGI_TEXTURED_GOURAUD_NOINVERSE_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_gouraud_noinverse_f.gxp")
set(LAGI_TEXTURED_GOURAUD_NOINVERSE_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_gouraud_noinverse_f_gxp.o")
set(LAGI_TEXTURED_GOURAUD_NOQUANT_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_gouraud_noquant_f.gxp")
set(LAGI_TEXTURED_GOURAUD_NOQUANT_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_gouraud_noquant_f_gxp.o")
set(LAGI_TEXTURED_RGB555_NOGOURAUD_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_rgb555_nogouraud_f.gxp")
set(LAGI_TEXTURED_RGB555_NOGOURAUD_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_rgb555_nogouraud_f_gxp.o")
set(LAGI_TEXTURED_GOURAUD_FINALQUANT_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_gouraud_finalquant_f.gxp")
set(LAGI_TEXTURED_GOURAUD_FINALQUANT_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_gouraud_finalquant_f_gxp.o")
set(LAGI_TEXTURED_GOURAUD_SCANLINE_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_gouraud_scanline_f.gxp")
set(LAGI_TEXTURED_GOURAUD_SCANLINE_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_gouraud_scanline_f_gxp.o")
set(LAGI_GOURAUD_SCANLINE_GRAY_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_gouraud_scanline_gray_f.gxp")
set(LAGI_GOURAUD_SCANLINE_GRAY_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_gouraud_scanline_gray_f_gxp.o")

set(LAGI_GOURAUD_PAYLOAD_V_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_gouraud_payload_v.gxp")
set(LAGI_GOURAUD_PAYLOAD_V_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_gouraud_payload_v_gxp.o")
set(LAGI_GOURAUD_SUBDIV_V_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_gouraud_subdiv_v.gxp")
set(LAGI_GOURAUD_SUBDIV_V_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_gouraud_subdiv_v_gxp.o")
set(LAGI_TEXTURED_GOURAUD_SUBDIV_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_gouraud_subdiv_f.gxp")
set(LAGI_TEXTURED_GOURAUD_SUBDIV_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_textured_gouraud_subdiv_f_gxp.o")
set(LAGI_GOURAUD_SUBDIV_GRAY_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_gouraud_subdiv_gray_f.gxp")
set(LAGI_GOURAUD_SUBDIV_GRAY_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_gouraud_subdiv_gray_f_gxp.o")

set(LAGI_TEXTURE_V_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_texture_v.gxp")
set(LAGI_TEXTURE_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_texture_f.gxp")
set(LAGI_TEXTURE_V_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_texture_v_gxp.o")
set(LAGI_TEXTURE_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_texture_f_gxp.o")
set(LAGI_MESH_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_mesh_f.gxp")
set(LAGI_MESH_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_mesh_f_gxp.o")

add_custom_command(
 OUTPUT "${LAGI_COLOR_V_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_vp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/lagi_color_v.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_COLOR_V_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/lagi_color_v.cg"
 COMMENT "Compiling Lagi color vertex shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_COLOR_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/lagi_color_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_COLOR_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/lagi_color_f.cg"
 COMMENT "Compiling Lagi color fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_COLOR_V_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_color_v.gxp" "${LAGI_COLOR_V_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_COLOR_V_GXP}"
 COMMENT "Embedding Lagi color vertex GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_COLOR_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_color_f.gxp" "${LAGI_COLOR_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_COLOR_F_GXP}"
 COMMENT "Embedding Lagi color fragment GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURE_V_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_vp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/texture_v.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_TEXTURE_V_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/texture_v.cg"
 COMMENT "Compiling Lagi texture vertex shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_GOURAUD_PAYLOAD_V_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_vp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/gouraud_payload_v.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_GOURAUD_PAYLOAD_V_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/gouraud_payload_v.cg"
 COMMENT "Compiling Lagi Gouraud payload vertex shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_GOURAUD_PAYLOAD_V_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_gouraud_payload_v.gxp" "${LAGI_GOURAUD_PAYLOAD_V_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_GOURAUD_PAYLOAD_V_GXP}"
 COMMENT "Embedding Lagi Gouraud payload vertex GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_GOURAUD_SUBDIV_V_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_vp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/gouraud_subdiv_v.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_GOURAUD_SUBDIV_V_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/gouraud_subdiv_v.cg"
 COMMENT "Compiling Lagi subdivided Gouraud vertex shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_GOURAUD_SUBDIV_V_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_gouraud_subdiv_v.gxp" "${LAGI_GOURAUD_SUBDIV_V_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_GOURAUD_SUBDIV_V_GXP}"
 COMMENT "Embedding Lagi subdivided Gouraud vertex GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_GOURAUD_SUBDIV_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_gouraud_subdiv_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_TEXTURED_GOURAUD_SUBDIV_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_gouraud_subdiv_f.cg"
 COMMENT "Compiling Lagi subdivided Gouraud fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_GOURAUD_SUBDIV_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_textured_gouraud_subdiv_f.gxp" "${LAGI_TEXTURED_GOURAUD_SUBDIV_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_TEXTURED_GOURAUD_SUBDIV_F_GXP}"
 COMMENT "Embedding Lagi subdivided Gouraud fragment GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_GOURAUD_SUBDIV_GRAY_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/gouraud_subdiv_gray_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_GOURAUD_SUBDIV_GRAY_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/gouraud_subdiv_gray_f.cg"
 COMMENT "Compiling Lagi subdivided Gouraud grayscale fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_GOURAUD_SUBDIV_GRAY_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_gouraud_subdiv_gray_f.gxp" "${LAGI_GOURAUD_SUBDIV_GRAY_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_GOURAUD_SUBDIV_GRAY_F_GXP}"
 COMMENT "Embedding Lagi subdivided Gouraud grayscale GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_GOURAUD_DEBUG_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/gouraud_debug_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_GOURAUD_DEBUG_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/gouraud_debug_f.cg"
 COMMENT "Compiling Lagi Gouraud debug fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_GOURAUD_DEBUG_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_gouraud_debug_f.gxp" "${LAGI_GOURAUD_DEBUG_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_GOURAUD_DEBUG_F_GXP}"
 COMMENT "Embedding Lagi Gouraud debug fragment GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_LIT_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_lit_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_TEXTURED_LIT_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_lit_f.cg"
 COMMENT "Compiling Lagi textured lighting fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_LIT_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_textured_lit_f.gxp" "${LAGI_TEXTURED_LIT_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_TEXTURED_LIT_F_GXP}"
 COMMENT "Embedding Lagi textured lighting fragment GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_LIT_OPAQUE_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_lit_opaque_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_TEXTURED_LIT_OPAQUE_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_lit_opaque_f.cg"
 COMMENT "Compiling Lagi opaque textured lighting fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_LIT_OPAQUE_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_textured_lit_opaque_f.gxp" "${LAGI_TEXTURED_LIT_OPAQUE_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_TEXTURED_LIT_OPAQUE_F_GXP}"
 COMMENT "Embedding Lagi opaque textured lighting fragment GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_LIT_HALF_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_lit_half_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_TEXTURED_LIT_HALF_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_lit_half_f.cg"
 COMMENT "Compiling Lagi half-precision exact textured lighting fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_LIT_HALF_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_textured_lit_half_f.gxp" "${LAGI_TEXTURED_LIT_HALF_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_TEXTURED_LIT_HALF_F_GXP}"
 COMMENT "Embedding Lagi half-precision exact textured lighting GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_LIT_NEWTON_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_lit_newton_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_TEXTURED_LIT_NEWTON_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_lit_newton_f.cg"
 COMMENT "Compiling Lagi Newton textured lighting fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_LIT_NEWTON_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_textured_lit_newton_f.gxp" "${LAGI_TEXTURED_LIT_NEWTON_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_TEXTURED_LIT_NEWTON_F_GXP}"
 COMMENT "Embedding Lagi Newton textured lighting fragment GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_PAYLOAD_PROBE_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_payload_probe_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_TEXTURED_PAYLOAD_PROBE_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_payload_probe_f.cg"
 COMMENT "Compiling Lagi heavy-payload texture probe fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_PAYLOAD_PROBE_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_textured_payload_probe_f.gxp" "${LAGI_TEXTURED_PAYLOAD_PROBE_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_TEXTURED_PAYLOAD_PROBE_F_GXP}"
 COMMENT "Embedding Lagi heavy-payload texture probe GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_GOURAUD_NOINVERSE_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_gouraud_noinverse_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_TEXTURED_GOURAUD_NOINVERSE_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_gouraud_noinverse_f.cg"
 COMMENT "Compiling Lagi no-inverse Gouraud fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_GOURAUD_NOINVERSE_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_textured_gouraud_noinverse_f.gxp" "${LAGI_TEXTURED_GOURAUD_NOINVERSE_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_TEXTURED_GOURAUD_NOINVERSE_F_GXP}"
 COMMENT "Embedding Lagi no-inverse Gouraud GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_GOURAUD_NOQUANT_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_gouraud_noquant_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_TEXTURED_GOURAUD_NOQUANT_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_gouraud_noquant_f.cg"
 COMMENT "Compiling Lagi Gouraud no-quantization fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_GOURAUD_NOQUANT_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_textured_gouraud_noquant_f.gxp" "${LAGI_TEXTURED_GOURAUD_NOQUANT_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_TEXTURED_GOURAUD_NOQUANT_F_GXP}"
 COMMENT "Embedding Lagi Gouraud no-quantization GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_RGB555_NOGOURAUD_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_rgb555_nogouraud_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_TEXTURED_RGB555_NOGOURAUD_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_rgb555_nogouraud_f.cg"
 COMMENT "Compiling Lagi RGB555 no-Gouraud fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_RGB555_NOGOURAUD_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_textured_rgb555_nogouraud_f.gxp" "${LAGI_TEXTURED_RGB555_NOGOURAUD_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_TEXTURED_RGB555_NOGOURAUD_F_GXP}"
 COMMENT "Embedding Lagi RGB555 no-Gouraud GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_GOURAUD_FINALQUANT_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_gouraud_finalquant_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_TEXTURED_GOURAUD_FINALQUANT_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_gouraud_finalquant_f.cg"
 COMMENT "Compiling Lagi final-quantization Gouraud fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_GOURAUD_FINALQUANT_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_textured_gouraud_finalquant_f.gxp" "${LAGI_TEXTURED_GOURAUD_FINALQUANT_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_TEXTURED_GOURAUD_FINALQUANT_F_GXP}"
 COMMENT "Embedding Lagi final-quantization Gouraud GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_GOURAUD_SCANLINE_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_gouraud_scanline_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_TEXTURED_GOURAUD_SCANLINE_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/textured_gouraud_scanline_f.cg"
 COMMENT "Compiling Lagi scanline Gouraud fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURED_GOURAUD_SCANLINE_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_textured_gouraud_scanline_f.gxp" "${LAGI_TEXTURED_GOURAUD_SCANLINE_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_TEXTURED_GOURAUD_SCANLINE_F_GXP}"
 COMMENT "Embedding Lagi scanline Gouraud GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_GOURAUD_SCANLINE_GRAY_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/gouraud_scanline_gray_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_GOURAUD_SCANLINE_GRAY_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/gouraud_scanline_gray_f.cg"
 COMMENT "Compiling Lagi scanline Gouraud grayscale fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_GOURAUD_SCANLINE_GRAY_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_gouraud_scanline_gray_f.gxp" "${LAGI_GOURAUD_SCANLINE_GRAY_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_GOURAUD_SCANLINE_GRAY_F_GXP}"
 COMMENT "Embedding Lagi scanline Gouraud grayscale GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURE_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/texture_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_TEXTURE_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/texture_f.cg"
 COMMENT "Compiling Lagi texture fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_MESH_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/mesh_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_MESH_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/mesh_f.cg"
 COMMENT "Compiling Lagi VDP1 mesh fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_MESH_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_mesh_f.gxp" "${LAGI_MESH_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_MESH_F_GXP}"
 COMMENT "Embedding Lagi VDP1 mesh fragment GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURE_V_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_texture_v.gxp" "${LAGI_TEXTURE_V_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_TEXTURE_V_GXP}"
 COMMENT "Embedding Lagi texture vertex GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_TEXTURE_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_texture_f.gxp" "${LAGI_TEXTURE_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_TEXTURE_F_GXP}"
 COMMENT "Embedding Lagi texture fragment GXP"
 VERBATIM
)

set_source_files_properties(
 "${LAGI_COLOR_V_OBJ}" "${LAGI_COLOR_F_OBJ}"
 "${LAGI_GOURAUD_DEBUG_F_OBJ}"
 "${LAGI_TEXTURED_LIT_F_OBJ}"
 "${LAGI_TEXTURED_LIT_OPAQUE_F_OBJ}"
 "${LAGI_TEXTURED_LIT_HALF_F_OBJ}"
 "${LAGI_TEXTURED_LIT_NEWTON_F_OBJ}"
 "${LAGI_TEXTURED_PAYLOAD_PROBE_F_OBJ}"
 "${LAGI_TEXTURED_GOURAUD_NOINVERSE_F_OBJ}"
 "${LAGI_TEXTURED_GOURAUD_NOQUANT_F_OBJ}"
 "${LAGI_TEXTURED_RGB555_NOGOURAUD_F_OBJ}"
 "${LAGI_TEXTURED_GOURAUD_FINALQUANT_F_OBJ}"
 "${LAGI_TEXTURED_GOURAUD_SCANLINE_F_OBJ}"
 "${LAGI_GOURAUD_SCANLINE_GRAY_F_OBJ}"
 "${LAGI_GOURAUD_PAYLOAD_V_OBJ}"
 "${LAGI_GOURAUD_SUBDIV_V_OBJ}"
 "${LAGI_TEXTURED_GOURAUD_SUBDIV_F_OBJ}"
 "${LAGI_GOURAUD_SUBDIV_GRAY_F_OBJ}"
 "${LAGI_TEXTURE_V_OBJ}" "${LAGI_TEXTURE_F_OBJ}"
 "${LAGI_MESH_F_OBJ}"
 PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE
)


# Compile the actual upstream Azel town runtime without modifying extern/Azel.
#
# Azel's PDS.h is a desktop umbrella header (SDL3/BGFX/SoLoud/ImGui/Tracy).
# Vita does not replace any runtime/gameplay code here; at configure time we
# generate build-only copies whose sole change is redirecting that umbrella
# include to Lagi's platform prelude. Every Azel function body remains
# byte-for-byte sourced from extern/Azel.
set(LAGI_AZEL_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib")
set(LAGI_AZEL_GENERATED_ROOT "${CMAKE_CURRENT_BINARY_DIR}/azel_upstream")
file(MAKE_DIRECTORY "${LAGI_AZEL_GENERATED_ROOT}")

function(lagi_prepare_upstream_azel_source relative_path out_var)
 set(input_path "${LAGI_AZEL_SOURCE_ROOT}/${relative_path}")
 if(NOT EXISTS "${input_path}")
  message(FATAL_ERROR "Missing upstream Azel source: ${input_path}")
 endif()

 file(READ "${input_path}" source_text)
 set(original_text "${source_text}")
 string(REPLACE
  "#include \"PDS.h\""
  "#include \"lagi/lagi_azel_upstream_prelude.h\""
  source_text
  "${source_text}"
 )

 # The Vita host owns filesystem access for files inside the mounted game
 # disc image. Keep the upstream desktop implementations available as weak
 # fallbacks in generated sources, while the strong Vita adapter supplies the
 # same engine API without changing extern/Azel.
 if(relative_path STREQUAL "PDS.cpp")
  string(REPLACE
   "bool findFileOnDisc(const std::string& filename)"
   "__attribute__((weak)) bool findFileOnDisc(const std::string& filename)"
   source_text "${source_text}")
  string(REPLACE
   "u32 getFileSize(const char* fileName)"
   "__attribute__((weak)) u32 getFileSize(const char* fileName)"
   source_text "${source_text}")
  string(REPLACE
   "int loadFile2(const char* fileName, u8* destination, u16 vdp1Pointer)"
   "__attribute__((weak)) int loadFile2(const char* fileName, u8* destination, u16 vdp1Pointer)"
   source_text "${source_text}")
  string(REPLACE
   "int loadFile(const char* fileName, u8* destination, u16 vdp1Pointer)"
   "__attribute__((weak)) int loadFile(const char* fileName, u8* destination, u16 vdp1Pointer)"
   source_text "${source_text}")
  string(REPLACE
   "int loadFile(const char* fileName, s_fileBundle** destination, u16 vdp1Pointer)"
   "__attribute__((weak)) int loadFile(const char* fileName, s_fileBundle** destination, u16 vdp1Pointer)"
   source_text "${source_text}")
  string(REPLACE
   "sSaturnMemoryFile::sSaturnMemoryFile(const char* fileName, u32 base)"
   "__attribute__((weak)) sSaturnMemoryFile::sSaturnMemoryFile(const char* fileName, u32 base)"
   source_text "${source_text}")
 elseif(relative_path STREQUAL "field.cpp")
  string(REPLACE
   "int findMandatoryFileOnDisc(const char* fileName)"
   "__attribute__((weak)) int findMandatoryFileOnDisc(const char* fileName)"
   source_text "${source_text}")
  string(REPLACE
   "u32 getFileSizeFromFileId(const char* fileName)"
   "__attribute__((weak)) u32 getFileSizeFromFileId(const char* fileName)"
   source_text "${source_text}")
 elseif(relative_path STREQUAL "town/ruin/twn_ruin_lock.cpp")
  # Task-owned Ruins lock/switch objects are drawn through Azel's normal
  # addObjectToDrawList() boundary, but pCurrentMatrix already contains the
  # town camera. Preserve their explicit world transform so Neptune applies
  # the camera only once. This patches only the generated build copy.
  string(REPLACE
   "#include \"lagi/lagi_azel_upstream_prelude.h\""
   "#include \"lagi/lagi_azel_upstream_prelude.h\"\n#include \"lagi/lagi_render_bridge.h\""
   source_text "${source_text}")
  string(REPLACE
   "void sLockTask::Draw(sLockTask* pThis)\n{\n    pushCurrentMatrix();\n    translateCurrentMatrix(pThis->m74_translation);\n    rotateCurrentMatrixZYX(pThis->m80_rotation);\n    addObjectToDrawList(pThis->m0_fileBundle->get3DModel(readSaturnU16(pThis->mC + 0x20)));\n    popMatrix();\n}"
   "void sLockTask::Draw(sLockTask* pThis)\n{\n    pushCurrentMatrix();\n    translateCurrentMatrix(pThis->m74_translation);\n    rotateCurrentMatrixZYX(pThis->m80_rotation);\n\n    sMatrix4x3 lagiWorldMatrix;\n    initMatrixToIdentity(&lagiWorldMatrix);\n    translateMatrix(pThis->m74_translation, &lagiWorldMatrix);\n    rotateMatrixZYX(&pThis->m80_rotation, &lagiWorldMatrix);\n\n    lagi::azel_bridge::SubmissionState lagiState{};\n    for (unsigned int lagiRow = 0; lagiRow < 3; ++lagiRow)\n    {\n        for (unsigned int lagiCol = 0; lagiCol < 4; ++lagiCol)\n        {\n            lagiState.modelMatrix[lagiRow * 4u + lagiCol] =\n                lagiWorldMatrix.m[lagiRow][lagiCol].asS32();\n        }\n    }\n    lagiState.hasModelMatrix = true;\n    lagiState.dynamic = true;\n    lagi::azel_bridge::capture_current_light(lagiState);\n\n    const u16 lagiModelOffset = readSaturnU16(pThis->mC + 0x20);\n    lagi::azel_bridge::set_town_submission_context(\n        -1,\n        static_cast<std::uint32_t>(pThis->mC.m_offset),\n        static_cast<std::uint32_t>(readSaturnU16(pThis->mC + 0x28)),\n        static_cast<std::uint32_t>(lagiModelOffset),\n        lagiState);\n\n    addObjectToDrawList(pThis->m0_fileBundle->get3DModel(lagiModelOffset));\n    popMatrix();\n}"
   source_text "${source_text}")
 elseif(relative_path STREQUAL "town/townMainLogic.cpp")
  # Correct two reconstructed hard-coded town LCS physical-button tests to
  # match the original manual: B cancels LCS, while A/C enter/select it.
  # Keep this in the generated build copy so extern/Azel remains untouched.
  string(REPLACE
   "if (graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown & 1)"
   "if (graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown & 2)"
   source_text "${source_text}")
 elseif(relative_path STREQUAL "town/townScript.cpp")
  string(REPLACE
   "if (graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown & 6)"
   "if (graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown & 5)"
   source_text "${source_text}")
 elseif(relative_path STREQUAL "town/town.cpp")
  # Vita bridge for the authentic upstream static world-grid submission path.
  # This modifies only the generated build copy, never extern/Azel.
  string(REPLACE
   "#include \"lagi/lagi_azel_upstream_prelude.h\""
   "#include \"lagi/lagi_azel_upstream_prelude.h\"\n#include \"lagi/lagi_render_bridge.h\""
   source_text "${source_text}")

  # Preserve Azel's pre-view world transform for static world-grid objects.
  # Neptune applies the authentic town camera later, so feeding it
  # pCurrentMatrix (view*cell*object) would apply the camera twice. Scope this
  # only to the static-cell addObjectToDrawList() boundary; actors/NPCs keep
  # their native runtime capture path.
  string(REPLACE
   "                        if (offset)\n                        {\n                            addObjectToDrawList(pThis->m0_fileBundle->get3DModel(offset));\n                        }"
   "                        if (offset)\n                        {\n                            sMatrix4x3 lagiWorldMatrix;\n                            initMatrixToIdentity(&lagiWorldMatrix);\n                            translateMatrix(pThis->mC_position, &lagiWorldMatrix);\n                            translateMatrix(readSaturnVec3(r14 + 4), &lagiWorldMatrix);\n\n                            sVec3_S16_12_4 lagiRotation;\n                            lagiRotation[0] = readSaturnS16(r14 + 0x10);\n                            lagiRotation[1] = readSaturnS16(r14 + 0x12);\n                            lagiRotation[2] = readSaturnS16(r14 + 0x14);\n                            rotateMatrixZYX_s16(lagiRotation, &lagiWorldMatrix);\n\n                            lagi::azel_bridge::SubmissionState lagiState{};\n                            for (unsigned int lagiRow = 0; lagiRow < 3; ++lagiRow)\n                            {\n                                for (unsigned int lagiCol = 0; lagiCol < 4; ++lagiCol)\n                                {\n                                    lagiState.modelMatrix[lagiRow * 4u + lagiCol] =\n                                        lagiWorldMatrix.m[lagiRow][lagiCol].asS32();\n                                }\n                            }\n                            lagiState.hasModelMatrix = true;\n                            lagiState.dynamic = false;\n\n                            lagi::azel_bridge::set_town_submission_context(\n                                -1,\n                                static_cast<std::uint32_t>(pThis->m8_cellPtr.m_offset),\n                                static_cast<std::uint32_t>(r14.m_offset),\n                                static_cast<std::uint32_t>(offset),\n                                lagiState);\n\n                            addObjectToDrawList(pThis->m0_fileBundle->get3DModel(offset));\n                        }"
   source_text "${source_text}")
 endif()
 if(source_text STREQUAL original_text)
  message(FATAL_ERROR
   "Upstream Azel source no longer contains the expected PDS.h include: "
   "${relative_path}")
 endif()

 string(REPLACE "/" "_" generated_name "${relative_path}")
 set(output_path "${LAGI_AZEL_GENERATED_ROOT}/${generated_name}")
 file(WRITE "${output_path}" "${source_text}")
 set_source_files_properties("${output_path}" PROPERTIES GENERATED TRUE)
 set(${out_var} "${output_path}" PARENT_SCOPE)
endfunction()

set(LAGI_AZEL_TOWN_RUNTIME_RELATIVE_SOURCES
 PDS.cpp
 common.cpp
 common_generated.cpp
 dummy.cpp
 trigo.cpp
 3dEngine.cpp
 VDP1.cpp
 3dModels.cpp
 processModel.cpp
 VDP2.cpp
 menu_dragonMorph.cpp
 dragonData.cpp
 mainMenuDebugTasks.cpp
 collision.cpp
 kernel/grid.cpp
 kernel/fileBundle.cpp
 kernel/graphicalObject.cpp
 kernel/animation.cpp
 kernel/vdp1Allocator.cpp
 kernel/fade.cpp
 kernel/cinematicBarsTask.cpp
 kernel/dialogTask.cpp
 kernel/receiveItemTask.cpp
 kernel/moduleManager.cpp
 kernel/loadSavegameScreen.cpp
 kernel/textDisplay.cpp
 items.cpp
 menu_dragon.cpp
 field.cpp
 field/field_a3/o_fld_a3.cpp
 shared/vdp2PlaneTask.cpp
 battle/BTL_A3/BTL_A3_map6.cpp
 audio/systemSounds.cpp
 town/townCutscene.cpp
 town/town.cpp
 town/townScript.cpp
 town/townEdge.cpp
 town/townMainLogic.cpp
 town/townCamera.cpp
 town/townLCS.cpp
 town/collisionRegistry.cpp
 kernel/worldGrid.cpp
 town/ruin/twn_ruin.cpp
 town/ruin/twn_ruin_lock.cpp
)

set(LAGI_AZEL_UPSTREAM_SOURCES)
foreach(relative_source IN LISTS LAGI_AZEL_TOWN_RUNTIME_RELATIVE_SOURCES)
 lagi_prepare_upstream_azel_source("${relative_source}" generated_source)
 list(APPEND LAGI_AZEL_UPSTREAM_SOURCES "${generated_source}")
endforeach()

add_executable(lagi
 src/main.cpp
 src/platform/vita/vita_platform.cpp
 src/platform/vita/vita_input.cpp
 src/platform/vita/vita_filesystem.cpp
 src/platform/vita/vita_log.cpp
 src/platform/vita/iso9660.cpp
 src/platform/vita/neptune_renderer.cpp
 ${LAGI_COLOR_V_OBJ}
 ${LAGI_COLOR_F_OBJ}
 ${LAGI_GOURAUD_DEBUG_F_OBJ}
 ${LAGI_TEXTURED_LIT_F_OBJ}
 ${LAGI_TEXTURED_LIT_OPAQUE_F_OBJ}
 ${LAGI_TEXTURED_LIT_HALF_F_OBJ}
 ${LAGI_TEXTURED_LIT_NEWTON_F_OBJ}
 ${LAGI_TEXTURED_PAYLOAD_PROBE_F_OBJ}
 ${LAGI_TEXTURED_GOURAUD_NOINVERSE_F_OBJ}
 ${LAGI_TEXTURED_GOURAUD_NOQUANT_F_OBJ}
 ${LAGI_TEXTURED_RGB555_NOGOURAUD_F_OBJ}
 ${LAGI_TEXTURED_GOURAUD_FINALQUANT_F_OBJ}
 ${LAGI_TEXTURED_GOURAUD_SCANLINE_F_OBJ}
 ${LAGI_GOURAUD_SCANLINE_GRAY_F_OBJ}
 ${LAGI_GOURAUD_PAYLOAD_V_OBJ}
 ${LAGI_GOURAUD_SUBDIV_V_OBJ}
 ${LAGI_TEXTURED_GOURAUD_SUBDIV_F_OBJ}
 ${LAGI_GOURAUD_SUBDIV_GRAY_F_OBJ}
 ${LAGI_TEXTURE_V_OBJ}
 ${LAGI_TEXTURE_F_OBJ}
 ${LAGI_MESH_F_OBJ}
 src/platform/vita/vita_audio.cpp
 src/movie/film_demuxer.cpp
 src/movie/cinepak_decoder.cpp
 src/movie/movie_player.cpp
 src/integration/azel_movie_bridge.cpp
 src/integration/lagi_runtime_smoke.cpp
 src/integration/lagi_render_bridge.cpp
 src/integration/lagi_live_model_adapter.cpp
 src/integration/lagi_direct_boot.cpp
 src/integration/lagi_town_bootstrap.cpp
 src/integration/lagi_town_runtime.cpp
 src/integration/lagi_town_collision.cpp
 src/integration/lagi_town_tasks.cpp
 src/integration/lagi_ruin_room_debug.cpp
 src/integration/lagi_edge_debug.cpp
 src/integration/lagi_fixedpoint_vita.cpp
 src/integration/lagi_heap_vita.cpp
 src/integration/lagi_task_vita.cpp
 src/integration/lagi_saturn_memory_vita.cpp
 src/integration/lagi_disc_file_vita.cpp
 src/integration/lagi_common_overlay_vita.cpp
 src/integration/lagi_sound_table_vita.cpp
 src/integration/lagi_dragon_common_vita.cpp
 src/integration/lagi_hotpoint_vita.cpp
 src/integration/lagi_audio_vita.cpp
 src/integration/lagi_renderer_host_vita.cpp
 ${LAGI_AZEL_UPSTREAM_SOURCES}
)
target_include_directories(lagi PRIVATE include
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/town"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/town/ruin"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/kernel"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/audio"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/shared"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/field/field_a3"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/battle/BTL_A3"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel"
)
target_compile_definitions(lagi PRIVATE LAGI_PLATFORM_VITA=1 LAGI_NEPTUNE_RENDERER=1 USE_NULL_RENDERER=1 SHIPPING_BUILD=1)
# Temporary bring-up aid only. The long-term target is the complete Azel
# runtime graph, not a Ruins-only binary. While enabled, this trims unreachable
# code so adapter work can proceed against the first playable scene without
# forcing every game subsystem to be integrated at once.
if(LAGI_AZEL_RUINS_LINK_TRIM)
 target_compile_options(lagi PRIVATE -ffunction-sections -fdata-sections)
 target_link_options(lagi PRIVATE -Wl,--gc-sections)
endif()

target_link_libraries(lagi m SceAudio_stub SceCtrl_stub SceDisplay_stub SceGxm_stub SceKernelThreadMgr_stub SceSysmem_stub SceSysmodule_stub)

if(LAGI_AZEL_PROBE)
 if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib")
  message(FATAL_ERROR "Azel submodule missing. Run: git submodule update --init --recursive")
 endif()
 add_library(lagi_azel_probe OBJECT src/integration/lagi_probe.cpp)
 target_include_directories(lagi_azel_probe PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib"
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel"
 )
 target_compile_definitions(lagi_azel_probe PRIVATE USE_NULL_RENDERER=1 LAGI_PLATFORM_VITA=1)

 add_library(lagi_azel_fixedpoint OBJECT
  src/integration/lagi_fixedpoint_vita.cpp
  src/integration/lagi_heap_vita.cpp
  src/integration/lagi_task_vita.cpp
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
vita_create_vpk(Lagi.vpk ${VITA_TITLEID} lagi.self
 VERSION ${VITA_VERSION}
 NAME ${VITA_APP_NAME}
 FILE
  "${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/icon0.png" sce_sys/icon0.png
  "${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/livearea/contents/bg0.png" sce_sys/livearea/contents/bg0.png
  "${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/livearea/contents/startup.png" sce_sys/livearea/contents/startup.png
  "${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/livearea/contents/template.xml" sce_sys/livearea/contents/template.xml
)
