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

set(VITA_APP_NAME "Lagi 0.040-alpha")
set(VITA_TITLEID "LAGI00001")
set(VITA_VERSION "00.40")
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
set(LAGI_CINEPAK_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_cinepak_f.gxp")
set(LAGI_CINEPAK_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_cinepak_f_gxp.o")
set(LAGI_VDP2_NBG_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_vdp2_nbg_f.gxp")
set(LAGI_VDP2_NBG_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_vdp2_nbg_f_gxp.o")
set(LAGI_VDP2_RBG0_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_vdp2_rbg0_f.gxp")
set(LAGI_VDP2_RBG0_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_vdp2_rbg0_f_gxp.o")

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
 OUTPUT "${LAGI_CINEPAK_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/cinepak_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_CINEPAK_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/cinepak_f.cg"
 COMMENT "Compiling Lagi SGX Cinepak fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_CINEPAK_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_cinepak_f.gxp" "${LAGI_CINEPAK_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_CINEPAK_F_GXP}"
 COMMENT "Embedding Lagi SGX Cinepak fragment GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_VDP2_NBG_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/vdp2_nbg_f.cg"
  -Wperf -fastprecision -O3
  -o "${LAGI_VDP2_NBG_F_GXP}"
 DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/vdp2_nbg_f.cg"
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/vdp2_common.cginc"
 COMMENT "Compiling Neptune VDP2 NBG fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_VDP2_NBG_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_vdp2_nbg_f.gxp" "${LAGI_VDP2_NBG_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_VDP2_NBG_F_GXP}"
 COMMENT "Embedding Neptune VDP2 NBG fragment GXP"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_VDP2_RBG0_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}"
  -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/vdp2_rbg0_f.cg"
  # RBG0 uses float values as exact packed-VRAM byte addresses and
  # coefficient accumulators. Do not allow PSP2CGC to reduce precision here.
  -Wperf -O3
  -o "${LAGI_VDP2_RBG0_F_GXP}"
 DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/vdp2_rbg0_f.cg"
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/vdp2_common.cginc"
 COMMENT "Compiling Neptune VDP2 RBG0 fragment shader"
 VERBATIM
)

add_custom_command(
 OUTPUT "${LAGI_VDP2_RBG0_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}"
  -I binary -O elf32-littlearm -B arm
  "lagi_vdp2_rbg0_f.gxp" "${LAGI_VDP2_RBG0_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_VDP2_RBG0_F_GXP}"
 COMMENT "Embedding Neptune VDP2 RBG0 fragment GXP"
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
 "${LAGI_CINEPAK_F_OBJ}"
 "${LAGI_VDP2_NBG_F_OBJ}" "${LAGI_VDP2_RBG0_F_OBJ}"
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

 # During Ruins-only bring-up, retain Azel's original module-manager state
 # machine and gameStatusTable, but do not make unintegrated overlays hard link
 # dependencies merely because they appear in overlayDispatchTable. The current
 # direct-boot task graph already owns TWN_RUIN; the only overlay it must
 # dispatch for 0.030-alpha is Azel's movie overlay. Once full-game overlay
 # integration lands, disabling LAGI_AZEL_RUINS_LINK_TRIM restores the upstream
 # dispatch table unchanged.
 if(relative_path STREQUAL "kernel/moduleManager.cpp" AND LAGI_AZEL_RUINS_LINK_TRIM)
  string(REPLACE
   "#include \"lagi/lagi_azel_upstream_prelude.h\""
   "#include \"lagi/lagi_azel_upstream_prelude.h\"\n#include \"lagi/platform.h\"\n#include \"lagi/azel_save_platform.h\""
   source_text "${source_text}")

  string(REPLACE
   "p_workArea(*overlayDispatchTable[])(p_workArea, s32) = {\n    loadMovieOverlay,   // 0: movie\n    loadTown,           // 1: town\n    loadTown2,          // 2: town (alt entry)\n    loadField,          // 3: field\n    loadBattle,         // 4: battle\n    loadWorldOverlay,   // 5: worldmap\n    NULL,               // 6: unused\n    NULL,               // 7: unused\n    NULL,               // 8: unused\n    initLoadSavegameScreen, // 9: save/load screen\n    NULL,\n};"
   "p_workArea(*overlayDispatchTable[])(p_workArea, s32) = {\n    loadMovieOverlay,   // 0: movie\n    loadTown,           // 1: town - authentic 0.040 boot path\n    loadTown2,          // 2: town alt - authentic 0.040 boot path\n    loadField,          // 3: field - D5 name entry\n    NULL,               // 4: battle - deferred until flight milestone\n    NULL,               // 5: worldmap - deferred\n    NULL,               // 6: unused\n    NULL,               // 7: unused\n    NULL,               // 8: unused\n    initLoadSavegameScreen, // 9: native Azel save/load screen\n    NULL,\n};"
   source_text "${source_text}")

  string(REPLACE
   "        if (gGameStatus.m6_previousGameStatus == 74)"
   "        if (gGameStatus.m6_previousGameStatus == 74 || lagiAzelSaveConsumeInGameLoad())"
   source_text "${source_text}")

  # Trace entry into module-manager state 1 before any transition helper.
  # Bracket-quoted CMake strings keep the generated C++ diagnostics readable
  # and avoid escaping ambiguity inside nested string literals.
  string(REPLACE
   [=[case 1:
        if ((gGameStatus.m8_nextGameStatus == 0) && pWorkArea->mC)]=]
   [=[case 1:
        lagi::platform::logging::writef("[AzelMM] enter state1 current=0x%02X next=0x%02X m2=%d mC=0x%08X\n", static_cast<unsigned int>(gGameStatus.m4_gameStatus), static_cast<unsigned int>(gGameStatus.m8_nextGameStatus), static_cast<int>(gGameStatus.m2), static_cast<unsigned int>(pWorkArea->mC));
        if ((gGameStatus.m8_nextGameStatus == 0) && pWorkArea->mC)]=]
   source_text "${source_text}")

  string(REPLACE
   [=[if ((gGameStatus.m8_nextGameStatus == 0) && pWorkArea->mC)
        {
            assert(0);
        }]=]
   [=[if ((gGameStatus.m8_nextGameStatus == 0) && pWorkArea->mC)
        {
            lagi::platform::logging::writef("[AzelMM] ASSERT path: next=0 and mC non-null\n");
            assert(0);
        }]=]
   source_text "${source_text}")

  string(REPLACE
   [=[else
        {
            if (gGameStatus.m2 == 0)]=]
   [=[else
        {
            lagi::platform::logging::writef("[AzelMM] state1 before status exit current=0x%02X m2=%d\n", static_cast<unsigned int>(gGameStatus.m4_gameStatus), static_cast<int>(gGameStatus.m2));
            if (gGameStatus.m2 == 0)]=]
   source_text "${source_text}")

  # Pinpoint the direct-boot town -> movie handoff. These diagnostics are
  # confined to the generated Vita copy and do not alter Azel decisions.
  string(REPLACE
   [=[case 4: // ruins
        setNextGameStatus(5);
        break;]=]
   [=[case 4: // ruins
        lagi::platform::logging::writef("[AzelMM] status4 before setNext(5)\n");
        setNextGameStatus(5);
        lagi::platform::logging::writef("[AzelMM] status4 after setNext next=0x%02X\n", static_cast<unsigned int>(gGameStatus.m8_nextGameStatus));
        break;]=]
   source_text "${source_text}")

  string(REPLACE
   [=[if ((gGameStatus.m2 == 0) && (gGameStatus.m6_previousGameStatus != 0x4F))
            {
                initFileLayoutTable();
            }]=]
   [=[if ((gGameStatus.m2 == 0) && (gGameStatus.m6_previousGameStatus != 0x4F))
            {
                lagi::platform::logging::writef("[AzelMM] before initFileLayoutTable next=0x%02X\n", static_cast<unsigned int>(gGameStatus.m8_nextGameStatus));
                initFileLayoutTable();
                lagi::platform::logging::writef("[AzelMM] after initFileLayoutTable\n");
            }]=]
   source_text "${source_text}")

  string(REPLACE
   [=[fadeOutAllSequences();
        case 0xC:]=]
   [=[lagi::platform::logging::writef("[AzelMM] before fadeOutAllSequences\n");
            fadeOutAllSequences();
            lagi::platform::logging::writef("[AzelMM] after fadeOutAllSequences\n");
        case 0xC:]=]
   source_text "${source_text}")

  string(REPLACE
   [=[fadePalette(&g_fadeControls.m0_fade0, convertColorToU32ForFade(g_fadeControls.m0_fade0.m0_color), 0, 30);
            fadePalette(&g_fadeControls.m24_fade1, convertColorToU32ForFade(g_fadeControls.m24_fade1.m0_color), 0, 30);]=]
   [=[lagi::platform::logging::writef("[AzelMM] before transition fadePalette\n");
            fadePalette(&g_fadeControls.m0_fade0, convertColorToU32ForFade(g_fadeControls.m0_fade0.m0_color), 0, 30);
            fadePalette(&g_fadeControls.m24_fade1, convertColorToU32ForFade(g_fadeControls.m24_fade1.m0_color), 0, 30);
            lagi::platform::logging::writef("[AzelMM] after transition fadePalette\n");]=]
   source_text "${source_text}")

  # The battle-manager override is irrelevant to the direct-boot Ruins/movie
  # path and would otherwise retain the unintegrated battle manager solely as
  # a linker dependency. Full builds keep the original condition.
  string(REPLACE
   "if ((readSaturnS8(gCommonFile->getSaturnPtr(0x212EAC + gGameStatus.m4_gameStatus * 2)) == 4) && (gBattleManager->mE == 2))"
   "if (false)"
   source_text "${source_text}")

 # Correct two incomplete VDP2 color-offset reconstructions in the generated
 # Vita copy. Saturn CLOFEN bit 0 is NBG0; m45_COEN must control that bit,
 # not the unrelated CCRNA ratio register.
 elseif(relative_path STREQUAL "VDP2.cpp")
  string(REPLACE
   "        case m45_COEN:\n            vdp2Controls.m4_pendingVdp2Regs->m108_CCRNA = (vdp2Controls.m4_pendingVdp2Regs->m108_CCRNA & 0xFFE0) | (arg << 0);\n            break;"
   "        case m45_COEN:\n            vdp2Controls.m4_pendingVdp2Regs->m110_CLOFEN = (vdp2Controls.m4_pendingVdp2Regs->m110_CLOFEN & ~0x1) | (arg << 0);\n            break;"
   source_text "${source_text}")

  # reinitVdp2() resets the register copies to zero, but the fade controller
  # intentionally survives across module/front-end setup. Republish the live
  # fade-channel colors immediately so newly constructed layers remain at the
  # current black/white offset during preload instead of flashing full-bright
  # until the next fade interrupt updates the registers.
  string(REPLACE
   "void reinitVdp2()\n{\n    vdp2Controls.m0_doubleBufferIndex = 0;\n    vdp2Controls.m4_pendingVdp2Regs = &vdp2Controls.m20_registers[0];\n\n    resetVdp2RegsCopies();"
   "void reinitVdp2()\n{\n    vdp2Controls.m0_doubleBufferIndex = 0;\n    vdp2Controls.m4_pendingVdp2Regs = &vdp2Controls.m20_registers[0];\n\n    resetVdp2RegsCopies();\n\n    const int fadeAR = g_fadeControls.m0_fade0.m0_color[0].getInteger();\n    const int fadeAG = g_fadeControls.m0_fade0.m0_color[1].getInteger();\n    const int fadeAB = g_fadeControls.m0_fade0.m0_color[2].getInteger();\n    const int fadeBR = g_fadeControls.m24_fade1.m0_color[0].getInteger();\n    const int fadeBG = g_fadeControls.m24_fade1.m0_color[1].getInteger();\n    const int fadeBB = g_fadeControls.m24_fade1.m0_color[2].getInteger();\n\n    for (int i = 0; i < 2; ++i) {\n        vdp2Controls.m20_registers[i].m114_COAR = fadeAR;\n        vdp2Controls.m20_registers[i].m116_COAG = fadeAG;\n        vdp2Controls.m20_registers[i].m118_COAB = fadeAB;\n        vdp2Controls.m20_registers[i].m11A_COBR = fadeBR;\n        vdp2Controls.m20_registers[i].m11C_COBG = fadeBG;\n        vdp2Controls.m20_registers[i].m11E_COBB = fadeBB;\n    }\n    vdp2Controls.m4_pendingVdp2Regs->m114_COAR = fadeAR;\n    vdp2Controls.m4_pendingVdp2Regs->m116_COAG = fadeAG;\n    vdp2Controls.m4_pendingVdp2Regs->m118_COAB = fadeAB;\n    vdp2Controls.m4_pendingVdp2Regs->m11A_COBR = fadeBR;\n    vdp2Controls.m4_pendingVdp2Regs->m11C_COBG = fadeBG;\n    vdp2Controls.m4_pendingVdp2Regs->m11E_COBB = fadeBB;\n    VDP2Regs_.m114_COAR = fadeAR;\n    VDP2Regs_.m116_COAG = fadeAG;\n    VDP2Regs_.m118_COAB = fadeAB;\n    VDP2Regs_.m11A_COBR = fadeBR;\n    VDP2Regs_.m11C_COBG = fadeBG;\n    VDP2Regs_.m11E_COBB = fadeBB;"
   source_text "${source_text}")

 # The title artwork is NBG0 and the title text is NBG1. Both participate in
 # the Saturn palette/color-offset fade, so make that intent explicit in the
 # reconstructed layer setup rather than faking a fullscreen renderer fade.
 elseif(relative_path STREQUAL "o_title.cpp")
  string(REPLACE
   "    m7_CNSM,  1,\n    m0_END, // Character Number Supplementary mode 1 (12bits)"
   "    m7_CNSM,  1,\n    m45_COEN, 1,\n    m0_END, // Character Number Supplementary mode 1 (12bits)"
   source_text "${source_text}")
  string(REPLACE
   "    m40_CAOS, 7, // palette offset is 7 * 0x200 = 0xE00 (where the palette for the text is stored)\n    m0_END,"
   "    m40_CAOS, 7, // palette offset is 7 * 0x200 = 0xE00 (where the palette for the text is stored)\n    m45_COEN, 1,\n    m0_END,"
   source_text "${source_text}")

 # The upstream Azel movie task/state machine remains authoritative. For the
 # Vita build copy only, remove the desktop Cinepak/SoLoud stream backend and
 # route those platform-facing calls through Lagi services. The filename table,
 # timing table, countdown, CPK sequencing, fades, skip logic, subtitle task,
 # and task completion all remain the original upstream code.
 elseif(relative_path STREQUAL "movie/movie.cpp")
  # The movie video is Saturn NBG0 and fadePalette() drives VDP2 color offset
  # A for that plane. Upstream's reconstructed config omitted COEN, leaving
  # CLOFEN at zero and forcing the Vita renderer to guess at the fade.
  string(REPLACE
   "    {m37_W0A,             1},\n    {m0_END,              0},"
   "    {m37_W0A,             1},\n    {m45_COEN,            1},\n    {m0_END,              0},"
   source_text "${source_text}")

  # MOVIE1 is the attract/title movie (movie timing entry 0x20). The current
  # reconstruction targets +white (0xFFFF) on exit, but Saturn hardware
  # reference fades to black before the title appears. Correct only this
  # reconstructed timing datum in the generated Vita copy; the native movie
  # task, fadePalette(), countdown, and transition logic remain authoritative.
  string(REPLACE
   "    {0x31, 0x01, 0xFFFF}, // 0x20"
   "    {0x31, 0x01, 0x8000}, // 0x20"
   source_text "${source_text}")

  string(REPLACE
   "#include \"movie.h\""
   "#include \"movie/movie.h\""
   source_text "${source_text}")
  string(REPLACE
   "#include \"cinepak.h\""
   "#include \"lagi/azel_movie_runtime_hooks.h\""
   source_text "${source_text}")
  string(REPLACE
   "#include \"movieAudio.h\""
   ""
   source_text "${source_text}")

  string(REPLACE
   "static GfsHn s_movieGfsHn;"
   "#if 0 // Lagi Vita: desktop movie stream backend replaced below\nstatic GfsHn s_movieGfsHn;"
   source_text "${source_text}")

  string(REPLACE
   "// ============================================================================\n// clearMovieVdp2Vram  –  06010584"
   "#endif // desktop movie stream backend\n\nstatic void openMovieStream(const char* cpkFileName)\n{\n    lagiAzelMovieStreamOpen(cpkFileName);\n}\n\nvoid closeMovieStream()\n{\n    lagiAzelMovieStreamClose();\n}\n\n// ============================================================================\n// clearMovieVdp2Vram  –  06010584"
   source_text "${source_text}")

  string(REPLACE
   "u32 lastUpdateFunction()\n{"
   "#if 0 // Lagi Vita: desktop decoder polling replaced below\nu32 lastUpdateFunction()\n{"
   source_text "${source_text}")

  string(REPLACE
   "// ============================================================================\n// createMovieDebugTask  –  0602a314"
   "#endif // desktop decoder polling\n\nu32 lastUpdateFunction()\n{\n    return static_cast<u32>(lagiAzelMovieLastUpdate());\n}\n\n// ============================================================================\n// createMovieDebugTask  –  0602a314"
   source_text "${source_text}")

 # The Vita host owns filesystem access for files inside the mounted game
 # disc image. Keep the upstream desktop implementations available as weak
 # fallbacks in generated sources, while the strong Vita adapter supplies the
 # same engine API without changing extern/Azel.
 elseif(relative_path STREQUAL "PDS.cpp")
  # Upstream labels this battle generic-data initialization as "not in
  # original". 0.040 now enters through Azel's native azelInit()/resetEngine()
  # path, so omit only that desktop reconstruction helper rather than making
  # boot depend on battle integration.
  string(REPLACE
   "    BTL_GenericData::staticInit(); // not in original"
   "    // Lagi Vita: desktop-only, explicitly non-original helper omitted."
   source_text "${source_text}")

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
 elseif(relative_path STREQUAL "mainMenuDebugTasks.cpp")
  # 0.040 boot-to-Ruins only needs the authentic D5 name-entry field before
  # town handoff. Do not retain references to later field overlays merely
  # because this large upstream translation unit contains the generic debug
  # dispatcher; those overlays are restored with their milestones.
  string(REPLACE
   "        createSiblingTaskWithArg<s_flagEditTaskWorkArea, p_workArea>(pThis->m8, pThis->m8);"
   "        // Lagi Vita 0.040: desktop/debug flag editor sibling omitted."
   source_text "${source_text}")

  # Pinned Azel leaves the System submenu constructor as a stub. Keep the
  # dispatch boundary but let Lagi provide the Vita task that enters Azel's
  # existing Save and Load tasks.
  string(REPLACE
   "p_workArea createSystemMenuTask(p_workArea workArea)"
   "__attribute__((weak)) p_workArea createSystemMenuTask(p_workArea workArea)"
   source_text "${source_text}")

  # Azel/Saturn work areas begin cleared. The Vita task host allocates C++
  # work areas with plain new, so explicitly establish the main status-menu
  # state before its first Draw() dispatch. Without this, m0 and child pointers
  # are indeterminate and pressing Start can enter an arbitrary menu state.
  string(REPLACE
   "void s_mainMenuWorkArea::Init(s_mainMenuWorkArea* pWorkArea)\n{\n    pWorkArea->m3_menuButtonStates[0] = 1; // item is always enabled"
   "void s_mainMenuWorkArea::Init(s_mainMenuWorkArea* pWorkArea)\n{\n    // LAGI_STATUS_MENU_ZERO_INIT\n    pWorkArea->m0 = 0;\n    pWorkArea->m1 = 0;\n    pWorkArea->selectedMenu = -1;\n    pWorkArea->m8 = nullptr;\n    pWorkArea->mC = nullptr;\n    pWorkArea->m10_cursorTask = nullptr;\n    pWorkArea->m3_menuButtonStates[0] = 1; // item is always enabled"
   source_text "${source_text}")

  string(REGEX MATCHALL
   "LAGI_STATUS_MENU_ZERO_INIT"
   lagi_status_menu_zero_init_matches
   "${source_text}")
  list(LENGTH
   lagi_status_menu_zero_init_matches
   lagi_status_menu_zero_init_match_count)
  if(NOT lagi_status_menu_zero_init_match_count EQUAL 1)
   message(FATAL_ERROR
    "Status-menu zero-init patch did not apply exactly once: found "
    "${lagi_status_menu_zero_init_match_count} matches")
  endif()

  string(REPLACE
   "        else if (!strcmp(pFieldDefinition->m_prg, \"FLD_A5.PRG\"))\n        {\n            gFieldOverlayFunction = overlayStart_FLD_A5;\n        }\n        else if (!strcmp(pFieldDefinition->m_prg, \"FLD_C8.PRG\"))\n        {\n            gFieldOverlayFunction = overlayStart_FLD_C8;\n        }\n        else if (!strcmp(pFieldDefinition->m_prg, \"FLD_D3.PRG\"))\n        {\n            gFieldOverlayFunction = overlayStart_FLD_D3;\n        }\n        else if (!strcmp(pFieldDefinition->m_prg, \"FLD_D5.PRG\"))"
   "        else if (!strcmp(pFieldDefinition->m_prg, \"FLD_D5.PRG\"))"
   source_text "${source_text}")

  string(REPLACE
   "        else if (!strcmp(pFieldDefinition->m_prg, \"FLD_A7.PRG\"))\n        {\n            gFieldOverlayFunction = overlayStart_FLD_A7;\n        }\n        else if (!strcmp(pFieldDefinition->m_prg, \"FLD_B2.PRG\"))\n        {\n            gFieldOverlayFunction = overlayStart_FLD_B2;\n        }"
   ""
   source_text "${source_text}")

  # Inventory and System are the currently backed native status-menu entries.
  # Keep Azel's real inventory task, but do not advertise the two constructors
  # that are still assert-only stubs in the pinned upstream revision.
  string(REPLACE
   "    if (mainGameState.gameStats.m20_XP) // defeated monsters menu\n    {\n        pWorkArea->m3_menuButtonStates[2] = 1;\n    }\n    else\n    {\n        pWorkArea->m3_menuButtonStates[2] = -1;\n    }\n    pWorkArea->m3_menuButtonStates[3] = 1; // map"
   "    pWorkArea->m3_menuButtonStates[2] = -1; // enemy list is an upstream stub\n    pWorkArea->m3_menuButtonStates[3] = -1; // map is an upstream stub"
   source_text "${source_text}")

  # First-flight milestone: keep Azel's native FLD_A3 dispatcher entry.
  # Phase 1 only restores reachability; rendering remains unchanged.
  string(REPLACE
   "            gFieldOverlayFunction = overlayStart_FLD_A3;"
   "            gFieldOverlayFunction = overlayStart_FLD_A3;\n            PDS_Log(\"[LagiFlight] selected FLD_A3 field=%d sub=%d entry=%d prevSub=%d\\n\", fieldTaskPtr->m2C_currentFieldIndex, fieldTaskPtr->m2E_currentSubFieldIndex, fieldTaskPtr->m30_fieldEntryPoint, fieldTaskPtr->m32_previousSubField);"
   source_text "${source_text}")

 elseif(relative_path STREQUAL "collision.cpp")
  # Collision visualization is desktop-only debug tooling. Do not retain
  # renderer-debug symbols in the shipping Vita build.
  string(REPLACE
   "                drawDebugFilledQuad(vector0, vector1, vector2, vector3, quadColor);"
   "                (void)quadColor;"
   source_text "${source_text}")
  string(REPLACE
   "                drawDebugLine(vector0, vector1, quadColor);\n                drawDebugLine(vector1, vector2, quadColor);\n                drawDebugLine(vector2, vector3, quadColor);\n                drawDebugLine(vector3, vector0, quadColor);"
   "                (void)quadColor;"
   source_text "${source_text}")

 elseif(relative_path STREQUAL "field/field_d5/o_fld_d5.cpp")
  # D5 intentionally reads shared camera/script data from FLD_A3.PRG. For the
  # 0.040 boot-to-Ruins milestone keep that exact Saturn data source without
  # instantiating FLD_A3_data, whose virtual gameplay surface pulls in the
  # entire first-flight overlay.
  string(REPLACE
   "    // FLD_A3.PRG contains shared field infrastructure data (camera configs, etc.)\n    if (gFLD_A3 == NULL)\n    {\n        gFLD_A3 = new FLD_A3_data();\n    }\n    gFieldCameraConfigEA = { 0x6092EF0, gFLD_A3 };\n    gFieldDragonAnimTableEA = { 0x06094134, gFLD_A3 };\n    gFieldCameraDrawFunc = &cameraFollowMode_default;\n    gCurrentFieldOverlay = gFLD_A3;"
   "    // FLD_A3.PRG contains shared field infrastructure data (camera configs, etc.)\n    static sFieldOverlay* lagiSharedA3Data = nullptr;\n    if (lagiSharedA3Data == nullptr)\n    {\n        lagiSharedA3Data = new sFieldOverlay(\"FLD_A3.PRG\");\n    }\n    gFieldCameraConfigEA = { 0x6092EF0, lagiSharedA3Data };\n    gFieldDragonAnimTableEA = { 0x06094134, lagiSharedA3Data };\n    gFieldCameraDrawFunc = &cameraFollowMode_default;\n    gCurrentFieldOverlay = lagiSharedA3Data;"
   source_text "${source_text}")

 elseif(relative_path STREQUAL "titleScreen.cpp")
  # Hardware-validated front-end sequencing correction for the generated Vita
  # copy. The reconstructed title menu currently queues the selected module in
  # the same frame that it starts the 30-frame fade to black. The root task
  # then replaces the title before that fade completes, allowing the incoming
  # movie to start its fade-to-neutral early. Real Saturn capture completes the
  # black fade first. Keep ownership in Azel: use title-menu state 2 solely to
  # wait for Azel's own fade control, then queue the already-selected task.
  string(REPLACE
   "                clearVdp2TextMemory();\n                initialTaskStatus.m_pendingTask = pWorkArea->m_menu[pWorkArea->m_currentSelection].m_createTask;\n\n                titleScreenDrawSub3(2);\n\n                // fade out to black\n                fadePalette(&g_fadeControls.m0_fade0, convertColorToU32ForFade(g_fadeControls.m0_fade0.m0_color), 0x8000, 30);\n                return;"
   "                clearVdp2TextMemory();\n\n                titleScreenDrawSub3(2);\n\n                // fade out to black\n                fadePalette(&g_fadeControls.m0_fade0, convertColorToU32ForFade(g_fadeControls.m0_fade0.m0_color), 0x8000, 30);\n                pWorkArea->m_status = 2;\n                return;"
   source_text "${source_text}")

  string(REPLACE
   "            break;\n\n        default:\n            assert(0);\n            break;\n        }\n    }\n\n    u16 m_status;"
   "            break;\n\n        case 2:\n            if (g_fadeControls.m0_fade0.m20_stopped == 0)\n                return;\n            initialTaskStatus.m_pendingTask = pWorkArea->m_menu[pWorkArea->m_currentSelection].m_createTask;\n            return;\n\n        default:\n            assert(0);\n            break;\n        }\n    }\n\n    u16 m_status;"
   source_text "${source_text}")

  # Upstream still has the Sega/Team Andromeda logo module as an assert-only
  # reconstruction stub. Until that static logo presentation is restored,
  # keep Azel's front-end attract loop alive by returning to the native title
  # movie wrapper instead of aborting after the title timeout.
  string(REPLACE
   "p_workArea startSegaLogoModule(p_workArea workArea)\n{\n    assert(false);\n    return NULL;\n}"
   "p_workArea startSegaLogoModule(p_workArea workArea)\n{\n    return startTitleScreenVideo(workArea);\n}"
   source_text "${source_text}")

 elseif(relative_path STREQUAL "field/fieldCamera.cpp")
  # Field Phase 4: capture Azel's exact active view matrix after it has been
  # applied to pCurrentMatrix. The bridge removes it from field submissions
  # and Neptune reapplies it once at presentation.
  string(REPLACE
   "#include \"lagi/lagi_azel_upstream_prelude.h\""
   "#include \"lagi/lagi_azel_upstream_prelude.h\"\n#include \"lagi/lagi_render_bridge.h\""
   source_text "${source_text}")
  string(REPLACE
   "    copyMatrix(pCurrentMatrix, &pTypedWorkArea->m384_viewMatrix);"
   "    copyMatrix(pCurrentMatrix, &pTypedWorkArea->m384_viewMatrix);\n    lagi::azel_bridge::capture_native_scene_view_matrix();"
   source_text "${source_text}")

 elseif(relative_path STREQUAL "field/field_a3/o_fld_a3.cpp")
  # Flight Phase 1 diagnostics: prove Azel reaches and initializes FLD_A3
  # before any field-specific Neptune presentation is enabled.
  string(REPLACE
   "bool initField(p_workArea workArea, const s_MCB_CGB* fieldFileList, u32 arg)\n{\n    reset3dEngine();"
   "bool initField(p_workArea workArea, const s_MCB_CGB* fieldFileList, u32 arg)\n{\n    PDS_Log(\"[LagiFlight] initField begin field=%d sub=%d arg=%u\\n\", getFieldTaskPtr()->m2C_currentFieldIndex, getFieldTaskPtr()->m2E_currentSubFieldIndex, (unsigned)arg);\n    reset3dEngine();"
   source_text "${source_text}")
  string(REPLACE
   "    loadCommonFieldResources();\n\n    setupFileList(fieldFileList);"
   "    loadCommonFieldResources();\n    PDS_Log(\"[LagiFlight] initField common-resources ready\\n\");\n\n    setupFileList(fieldFileList);\n    PDS_Log(\"[LagiFlight] initField file-list ready\\n\");"
   source_text "${source_text}")
  string(REPLACE
   "    loadFileFromFileList(0);\n    createFieldScriptTask(workArea);\n    createFieldOverlaySubTask(workArea);\n    initFieldDragon(workArea, arg);\n    createFieldOverlaySubTask2(workArea);\n    createFieldCameraTask(workArea);\n    createRandomBattleTask(workArea);\n    resetProjectVector();"
   "    loadFileFromFileList(0);\n    PDS_Log(\"[LagiFlight] initField file0 loaded\\n\");\n    createFieldScriptTask(workArea);\n    PDS_Log(\"[LagiFlight] initField script-task created\\n\");\n    createFieldOverlaySubTask(workArea);\n    PDS_Log(\"[LagiFlight] initField overlay-task created\\n\");\n    initFieldDragon(workArea, arg);\n    PDS_Log(\"[LagiFlight] initField dragon-task created\\n\");\n    createFieldOverlaySubTask2(workArea);\n    PDS_Log(\"[LagiFlight] initField overlay-task2 created\\n\");\n    createFieldCameraTask(workArea);\n    PDS_Log(\"[LagiFlight] initField camera-task created\\n\");\n    createRandomBattleTask(workArea);\n    PDS_Log(\"[LagiFlight] initField encounter-task created\\n\");\n    resetProjectVector();"
   source_text "${source_text}")
  string(REPLACE
   "    pauseEngine[2] = 1;\n\n    return true;"
   "    pauseEngine[2] = 1;\n    PDS_Log(\"[LagiFlight] initField complete\\n\");\n\n    return true;"
   source_text "${source_text}")
  string(REPLACE
   "p_workArea overlayStart_FLD_A3(p_workArea workArea, u32 arg)\n{\n    // Custom loader thing"
   "p_workArea overlayStart_FLD_A3(p_workArea workArea, u32 arg)\n{\n    PDS_Log(\"[LagiFlight] overlayStart_FLD_A3 enter field=%d sub=%d entry=%d prevSub=%d arg=%u\\n\", getFieldTaskPtr()->m2C_currentFieldIndex, getFieldTaskPtr()->m2E_currentSubFieldIndex, getFieldTaskPtr()->m30_fieldEntryPoint, getFieldTaskPtr()->m32_previousSubField, (unsigned)arg);\n    // Custom loader thing"
   source_text "${source_text}")
  string(REPLACE
   "    if (!initField(workArea, fieldFileList, arg))\n    {\n        return NULL;\n    }"
   "    if (!initField(workArea, fieldFileList, arg))\n    {\n        PDS_Log(\"[LagiFlight] overlayStart_FLD_A3 initField FAILED\\n\");\n        return NULL;\n    }\n    PDS_Log(\"[LagiFlight] overlayStart_FLD_A3 initField ok\\n\");"
   source_text "${source_text}")
  string(REPLACE
   "    //0605404C\n    loadFileFromFileList(1);"
   "    //0605404C\n    loadFileFromFileList(1);\n    PDS_Log(\"[LagiFlight] overlayStart_FLD_A3 secondary field file loaded\\n\");"
   source_text "${source_text}")

  # Azel/Saturn work areas begin cleared. Make the field-script task's
  # no-active-script invariant explicit on Vita so New Game cannot enter the
  # interpreter with stale/poisoned state before a script is queued.
  string(REPLACE
   "void s_fieldScriptWorkArea::Init(s_fieldScriptWorkArea* pFieldScriptWorkArea)\n{\n    s_fieldTaskWorkArea* pFieldTaskWorkArea = getFieldTaskPtr();"
   "void s_fieldScriptWorkArea::Init(s_fieldScriptWorkArea* pFieldScriptWorkArea)\n{\n    pFieldScriptWorkArea->m4_currentScript = sSaturnPtr::getNull();\n    pFieldScriptWorkArea->m30_cinematicBarTask = nullptr;\n    pFieldScriptWorkArea->m34 = 0;\n    pFieldScriptWorkArea->m38_dialogStringTask = nullptr;\n    pFieldScriptWorkArea->m3C_multichoiceTask = nullptr;\n    pFieldScriptWorkArea->m40_receivedItemTask = nullptr;\n    pFieldScriptWorkArea->m44_multiChoiceData = nullptr;\n    pFieldScriptWorkArea->m48_cutsceneTask = nullptr;\n    pFieldScriptWorkArea->m4C_PCMPlaying = 0;\n    pFieldScriptWorkArea->m50_scriptDelay = 0;\n    pFieldScriptWorkArea->m54_currentResult = 0;\n    pFieldScriptWorkArea->m58 = 0;\n    pFieldScriptWorkArea->m5C = 0;\n    pFieldScriptWorkArea->m60_canSkipScript = 0;\n    pFieldScriptWorkArea->m64 = 0;\n    pFieldScriptWorkArea->m80 = nullptr;\n    pFieldScriptWorkArea->m84 = nullptr;\n    s_fieldTaskWorkArea* pFieldTaskWorkArea = getFieldTaskPtr();"
   source_text "${source_text}")

  # Field scripts own the decision to play PCM; Lagi supplies the platform
  # service. Avoid the upstream desktop SoLoud backend on Vita.
  string(REPLACE
   "#include \"lagi/lagi_azel_upstream_prelude.h\""
   "#include \"lagi/lagi_azel_upstream_prelude.h\"\n#include \"lagi/lagi_render_bridge.h\"\nextern void lagiAzelFieldPlayPCM(const char* filename);"
   source_text "${source_text}")
  string(REPLACE
   "    playPCMFile(pPcmName);"
   "    lagiAzelFieldPlayPCM(pPcmName);"
   source_text "${source_text}")
  # Phase 4 static field-grid bridge. Azel still performs clipping and LOD
  # selection; Lagi only publishes the selected rigid environment object with
  # its stable pre-view world transform.
  string(REPLACE
   "                if (r14->m0_modelsOffets[depthRangeIndex])\n                {\n                    sProcessed3dModel* pModel = pTypedWorkAread->m0_memoryLayout.m0_mainMemoryBundle->get3DModel(r14->m0_modelsOffets[depthRangeIndex]);\n                    addObjectToDrawList(pModel);\n                }"
   "                if (r14->m0_modelsOffets[depthRangeIndex])\n                {\n                    const u32 lagiModelOffset = r14->m0_modelsOffets[depthRangeIndex];\n                    sProcessed3dModel* pModel = pTypedWorkAread->m0_memoryLayout.m0_mainMemoryBundle->get3DModel(lagiModelOffset);\n\n                    sMatrix4x3 lagiWorldMatrix;\n                    initMatrixToIdentity(&lagiWorldMatrix);\n                    translateMatrix(&r14->m4, &lagiWorldMatrix);\n                    rotateMatrixZYX_s16(r14->m10, &lagiWorldMatrix);\n\n                    lagi::azel_bridge::SubmissionState lagiState{};\n                    for (unsigned int lagiRow = 0; lagiRow < 3; ++lagiRow)\n                    {\n                        for (unsigned int lagiCol = 0; lagiCol < 4; ++lagiCol)\n                        {\n                            lagiState.modelMatrix[lagiRow * 4u + lagiCol] = lagiWorldMatrix.m[lagiRow][lagiCol].asS32();\n                        }\n                    }\n                    lagiState.hasModelMatrix = true;\n                    lagiState.dynamic = false;\n                    lagi::azel_bridge::capture_current_light(lagiState);\n                    // LAGI_FLD_A3_STATIC_IDENTITY\n                    lagi::azel_bridge::set_town_submission_context(\n                        -1,\n                        static_cast<std::uint32_t>(pTypedWorkAread->m14_index),\n                        static_cast<std::uint32_t>(r14->EA.m_offset),\n                        static_cast<std::uint32_t>(lagiModelOffset),\n                        lagiState);\n\n                    addObjectToDrawList(pModel);\n                }"
   source_text "${source_text}")

  # o_fld_a3.cpp receives several independent generated-source patches. The
  # whole-file changed check below cannot prove that this performance-critical
  # draw-boundary replacement matched, so require its unique injected call
  # exactly once before allowing configuration to continue.
  string(REGEX MATCHALL
   "LAGI_FLD_A3_STATIC_IDENTITY"
   lagi_field_grid_static_matches
   "${source_text}")
  list(LENGTH
   lagi_field_grid_static_matches
   lagi_field_grid_static_match_count)
  if(NOT lagi_field_grid_static_match_count EQUAL 1)
   message(FATAL_ERROR
    "FLD_A3 static grid submission patch did not apply exactly once: "
    "found ${lagi_field_grid_static_match_count} matches")
  endif()

  # Upstream rayDisplay.cpp is BGFX/GLM desktop renderer code. FLD_A3 is now
  # reachable for flight bring-up, but native ray/laser presentation belongs
  # to a later Neptune phase. Keep only those desktop draw branches suppressed.
  string(REPLACE
   "        if (isGunShotVisible(segment, graphicEngineStatus.m405C))"
   "        if (false)"
   source_text "${source_text}")
  string(REPLACE
   "            displayRaySegmentFromViewSpace(segment, 0x333, vdp1Base + 0xF8, 0x210, vdp1Base + 0x173C, &gouraud, 8);"
   "            /* Lagi flight bring-up: field laser ray draw deferred to native Neptune path */"
   source_text "${source_text}")
  string(REPLACE
   "    if (gDirectRayRendering)"
   "    if (false)"
   source_text "${source_text}")
  string(REPLACE
   "            displayRaySegmentFromViewSpace(segment, width, characterAddress, characterSize, characterColor, &(*arg0)[i], 8);"
   "            /* Lagi flight bring-up: field laser ray draw deferred to native Neptune path */"
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
 elseif(relative_path STREQUAL "battle/battleOverlay.cpp")
  # Battle manager stays authentic, but no battle is reachable before the
  # first-flight milestone. Remove only executable battle-overlay references;
  # keep declarations and surrounding task structure valid.
  string(REPLACE
   "        BattleEnd_deleteSub0();"
   "        /* Lagi 0.040: battle teardown deferred until battle milestone */"
   source_text "${source_text}")
  string(REGEX REPLACE
   "                overlayStart_BTL_[A-Za-z0-9_]+\\(pThis\\);"
   "                /* Lagi 0.040: battle overlay dispatch deferred */"
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
 elseif(relative_path STREQUAL "town/townEdge.cpp")
  # Edge's native hierarchy draw path submits through addObjectToDrawList()
  # with pCurrentMatrix already containing the town camera. Mark the scope so
  # Lagi's render bridge strips only that view transform and publishes each
  # animated hierarchy node in world space. The upstream source is unchanged.
  string(REPLACE
   "#include \"lagi/lagi_azel_upstream_prelude.h\""
   "#include \"lagi/lagi_azel_upstream_prelude.h\"\n#include \"lagi/lagi_render_bridge.h\""
   source_text "${source_text}")
  string(REPLACE
   "void sEdgeTask::Draw(sEdgeTask* pThis)\n{\n    pushCurrentMatrix();"
   "void sEdgeTask::Draw(sEdgeTask* pThis)\n{\n    lagi::azel_bridge::begin_view_relative_submission_scope();\n    pushCurrentMatrix();"
   source_text "${source_text}")
  string(REPLACE
   "    popMatrix();\n}\n\nvoid sEdgeTask::Delete"
   "    popMatrix();\n    lagi::azel_bridge::end_view_relative_submission_scope();\n}\n\nvoid sEdgeTask::Delete"
   source_text "${source_text}")
 elseif(relative_path STREQUAL "town/townCutscene.cpp")
  # Generic Vita bring-up for Azel's native in-engine town cutscene subsystem.
  # Azel retains ownership of script ordering, fades, camera commands, entity
  # animation, subtitles and completion. Lagi supplies only disc transport,
  # task allocator semantics, a stable 30 Hz timeline and lifecycle cleanup.
  string(REPLACE
   "#include \"lagi/lagi_azel_upstream_prelude.h\""
   "#include \"lagi/lagi_azel_upstream_prelude.h\"\n#include \"lagi/disc_image.h\"\n#include \"lagi/platform.h\"\n#include <algorithm>\n#include <cstdint>\n#include <cstring>"
   source_text "${source_text}")

  # Replace desktop FILE* access with the mounted ISO9660 random-access layer.
  string(REPLACE
   [=[GfsHn GFS_Open(const std::string& name)
{
    GfsHn temp;
    temp.fHandle = fopen(name.c_str(), "rb");
    return temp;
}]=]
   [=[struct LagiGfsHandle
{
    lagi::disc::FileHandle file;
    std::uint64_t offset = 0;
};

static LagiGfsHandle* lagiGfsHandle(GfsHn handle)
{
    return reinterpret_cast<LagiGfsHandle*>(handle.fHandle);
}

GfsHn GFS_Open(const std::string& name)
{
    GfsHn temp{};
    auto* handle = new LagiGfsHandle{};
    if (!lagi::disc::open_file(name.c_str(), handle->file))
    {
        lagi::platform::logging::writef(
            "[AzelCutscene] missing stream file: %s\\n", name.c_str());
        delete handle;
        return temp;
    }

    temp.fHandle = reinterpret_cast<FILE*>(handle);
    lagi::platform::logging::writef(
        "[AzelCutscene] opened %s (%u bytes) from mounted disc\\n",
        name.c_str(), static_cast<unsigned>(handle->file.size));
    return temp;
}]=]
   source_text "${source_text}")

  string(REPLACE
   [=[void GFS_GetFileSize(GfsHn handle, s32* sectorSize, s32* numSector, s32* lastSize)
{
    u32 position = ftell(handle.fHandle);

    fseek(handle.fHandle, 0, SEEK_END);
    u32 size = ftell(handle.fHandle);

    fseek(handle.fHandle, position, SEEK_SET);

    *sectorSize = 0x800;
    *numSector = size / 0x800;
    *lastSize = size - (*numSector * 0x800);
}]=]
   [=[void GFS_GetFileSize(GfsHn handle, s32* sectorSize, s32* numSector, s32* lastSize)
{
    const LagiGfsHandle* stream = lagiGfsHandle(handle);
    const u32 size = stream ? stream->file.size : 0;
    *sectorSize = 0x800;
    *numSector = static_cast<s32>((size + 0x7ffu) / 0x800u);
    *lastSize = *numSector
        ? static_cast<s32>(size - static_cast<u32>(*numSector - 1) * 0x800u)
        : 0;
}]=]
   source_text "${source_text}")

  string(REPLACE
   [=[s32 GFS_NwFread(GfsHn handle, s32 numSectors, u8* buffer, s32 bufferSize)
{
    gNumSectors = numSectors;
    assert(bufferSize >= numSectors * 0x800);
    numReadBytes = numSectors * 0x800;
    return fread(buffer, numSectors, 0x800, handle.fHandle);
}]=]
   [=[s32 GFS_NwFread(GfsHn handle, s32 numSectors, u8* buffer, s32 bufferSize)
{
    LagiGfsHandle* stream = lagiGfsHandle(handle);
    const s32 requestBytes = numSectors * 0x800;
    gNumSectors = numSectors;
    assert(bufferSize >= requestBytes);
    if (!stream || !buffer || requestBytes < 0)
        return -1;

    const std::uint64_t size = stream->file.size;
    const std::uint64_t remaining =
        stream->offset < size ? size - stream->offset : 0;
    const std::size_t available = static_cast<std::size_t>(
        std::min<std::uint64_t>(
            remaining, static_cast<std::uint64_t>(requestBytes)));

    if (available &&
        !lagi::disc::read_file_at(
            stream->file, stream->offset, buffer, available))
        return -1;

    if (available < static_cast<std::size_t>(requestBytes))
        std::memset(
            buffer + available, 0,
            static_cast<std::size_t>(requestBytes) - available);

    stream->offset += static_cast<std::uint64_t>(requestBytes);
    numReadBytes = requestBytes;
    return numSectors;
}]=]
   source_text "${source_text}")

  # The Saturn work-area allocator clears task memory before Init. Recreate
  # that contract explicitly and reserve the stream's 0x8000-byte PCM scratch
  # area alongside the 0x20000-byte command ring buffer.
  string(REPLACE
   [=[void sCutsceneTask::Init(sCutsceneTask* pThis, s32 arg)
{
    pThis->m1D4 = dramAllocate(0x20000);

    sStreamingParams streamingParams;
    streamingParams.m0_streamingFile = &pThis->m4;
    streamingParams.m4_streamingFileSize = sizeof(pThis->m4);
    streamingParams.m8_buffer = pThis->m1D4;
    streamingParams.mC_bufferSize = 0x20000;
    streamingParams.m10_audioBuffer = nullptr;//TODO: figure this out
    streamingParams.m14_audioBufferSize = 0x8000;

    std::string streamFileName = readSaturnString(sSaturnPtr::createFromRaw(arg, gCurrentTownOverlay));
    findMandatoryFileOnDisc(streamFileName.c_str());
    pThis->m0 = openFileForStreaming(&streamingParams, streamFileName);

}]=]
   [=[void sCutsceneTask::Init(sCutsceneTask* pThis, s32 arg)
{
    pThis->m0 = nullptr;
    pThis->m4 = sStreamingFile{};
    pThis->m1D4 = dramAllocate(0x28000);
    if (!pThis->m1D4)
    {
        lagi::platform::logging::writef("[AzelCutscene] stream allocation failed\\n");
        pThis->m4.m28.m0 = -1;
        pThis->m0 = &pThis->m4;
        return;
    }

    sStreamingParams streamingParams{};
    streamingParams.m0_streamingFile = &pThis->m4;
    streamingParams.m4_streamingFileSize = sizeof(pThis->m4);
    streamingParams.m8_buffer = pThis->m1D4;
    streamingParams.mC_bufferSize = 0x20000;
    streamingParams.m10_audioBuffer = pThis->m1D4 + 0x20000;
    streamingParams.m14_audioBufferSize = 0x8000;

    std::string streamFileName =
        readSaturnString(sSaturnPtr::createFromRaw(arg, gCurrentTownOverlay));
    lagi::platform::logging::writef(
        "[AzelCutscene] request %s arg=%08X\\n",
        streamFileName.c_str(), static_cast<unsigned>(arg));

    pThis->m0 = openFileForStreaming(&streamingParams, streamFileName);
    if (!pThis->m0)
    {
        lagi::platform::logging::writef(
            "[AzelCutscene] open failed: %s\\n", streamFileName.c_str());
        pThis->m4.m28.m0 = -1;
        pThis->m0 = &pThis->m4;
    }
}]=]
   source_text "${source_text}")

  # Generic close/completion path. Every town script already waits on state 5.
  string(REPLACE
   [=[void updateStreamingFileReadSub0(sStreamingFile* iParm1)
{
    Unimplemented();
}]=]
   [=[void updateStreamingFileReadSub0(sStreamingFile* iParm1)
{
    if (!iParm1)
        return;

    LagiGfsHandle* stream = lagiGfsHandle(iParm1->m198.m0_gfsHandle);
    if (stream)
    {
        lagi::disc::close_file(stream->file);
        delete stream;
        iParm1->m198.m0_gfsHandle.fHandle = nullptr;
    }

    iParm1->m198.m8_readIsPending = 0;
    iParm1->m28.m0 = 5;
}]=]
   source_text "${source_text}")

  # Keep entity registration safe during failed/tearing-down streams.
  string(REPLACE
   "void setupCutsceneDragonSub0(sStreamingFile* r4, u32 index, u32 param_3, p_workArea param_4, s_3dModel* pModel)\n{\n    if (index < 0x11)"
   "void setupCutsceneDragonSub0(sStreamingFile* r4, u32 index, u32 param_3, p_workArea param_4, s_3dModel* pModel)\n{\n    if (!r4) return;\n    if (index < 0x11)"
   source_text "${source_text}")

  string(REPLACE
   [=[s32 scriptFunction_605861eSub0Sub0(sStreamingFile* r4)
{
    return r4->m28.m0;
}]=]
   [=[s32 scriptFunction_605861eSub0Sub0(sStreamingFile* r4)
{
    return r4 ? r4->m28.m0 : -1;
}]=]
   source_text "${source_text}")

  # Upstream reconstruction still has Saturn timer/audio service stubs. Use the
  # actual stream frame-rate against Lagi's 30 Hz game tick so Azel's command
  # timeline, camera, subtitles and animation remain data-driven.
  string(REPLACE
   [=[void syncCutsceneTimer(sStreamingFile* psParm1)
{
    Unimplemented();
}]=]
   [=[void syncCutsceneTimer(sStreamingFile* psParm1)
{
    if (!psParm1 || psParm1->m28.m0 != 4 || pauseEngine[0])
        return;

    const u32 frameRate =
        (psParm1->m194 && psParm1->m194->mC_frameRate)
            ? psParm1->m194->mC_frameRate : 60u;

    psParm1->m28.m7C += frameRate;
    while (psParm1->m28.m7C >= 30u)
    {
        ++psParm1->m28.m84_frameIndex;
        psParm1->m28.m7C -= 30u;
    }
}]=]
   source_text "${source_text}")

  string(REPLACE
   [=[            if ((psParm1->m28).m8 == 0) {
                Unimplemented();

                // hack!
                (psParm1->m28).m84_frameIndex+=2;
                //assert(0);
                /*
]=]
   [=[            if ((psParm1->m28).m8 == 0) {
                /*
]=]
   source_text "${source_text}")

  # These Saturn-side hooks do not own sequencing. Keep them benign while the
  # native Azel parser advances commands and the Vita timeline drives frames.
  string(REPLACE
   [=[void cutsceneCommand0Sub1(sStreamingFile* param_1)
{
    assert(0);
}]=]
   [=[void cutsceneCommand0Sub1(sStreamingFile*)
{
}]=]
   source_text "${source_text}")

  string(REPLACE
   [=[s32 cutsceneCommand1(sStreamingFile* param_1)
{
    Unimplemented();
    return 0;
}]=]
   [=[s32 cutsceneCommand1(sStreamingFile*)
{
    // Opcode 1 is Saturn streamed-audio service data. Its packet remains part
    // of Azel's command stream; Vita currently advances it while the game's
    // native SCSP/BGM service continues independently.
    return 0;
}]=]
   source_text "${source_text}")

  string(REPLACE
   [=[void pauseCutscene(sStreamingFile* r4, s32 r5)
{
    Unimplemented();
}]=]
   [=[void pauseCutscene(sStreamingFile*, s32)
{
    // pauseEngine[] already gates the Vita cutscene timeline.
}]=]
   source_text "${source_text}")

  string(REPLACE
   [=[
    Unimplemented();
}

void pauseCutscene]=]
   [=[
}

void pauseCutscene]=]
   source_text "${source_text}")

  # Validate this generated-source bridge as one unit so future upstream edits
  # fail configuration rather than silently reverting to desktop/stub behavior.
  string(REGEX MATCHALL
   "LagiGfsHandle|stream allocation failed|pauseEngine\\[0\\] already gates"
   lagi_cutscene_bridge_markers "${source_text}")
  list(LENGTH lagi_cutscene_bridge_markers lagi_cutscene_bridge_marker_count)
  if(lagi_cutscene_bridge_marker_count LESS 3)
   message(FATAL_ERROR
    "Native town cutscene bridge did not apply completely; upstream Azel changed")
  endif()
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
  # First-flight progression reaches Azel's native E006 cinematic and then
  # the Excavation town. Preserve both authentic dispatches; later unrelated
  # town overlays remain trimmed until their own dependencies are integrated.
  string(REPLACE
   "    else if (overlayFileName == \"TWN_EXCA.PRG\")\n    {\n        gFieldOverlayFunction = overlayStart_TWN_EXCA;\n    }\n    else if (overlayFileName == \"TWN_E006.PRG\")\n    {\n        gFieldOverlayFunction = overlayStart_TWN_E006;\n    }\n    else if (overlayFileName == \"TWN_E011.PRG\")\n    {\n        gFieldOverlayFunction = overlayStart_TWN_E011;\n    }\n    else if (overlayFileName == \"TWN_E014.PRG\")\n    {\n        gFieldOverlayFunction = overlayStart_TWN_E014;\n    }\n    else if (overlayFileName == \"TWN_CAMP.PRG\")\n    {\n        gFieldOverlayFunction = overlayStart_TWN_CAMP;\n    }\n    else if (overlayFileName == \"TWN_ZOAH.PRG\")\n    {\n        gFieldOverlayFunction = overlayStart_TWN_ZOAH;\n    }\n    else if (overlayFileName == \"TWN_CARA.PRG\")\n    {\n        gFieldOverlayFunction = overlayStart_TWN_CARA;\n    }\n    else if (overlayFileName == \"TWN_SEEK.PRG\")\n    {\n        gFieldOverlayFunction = overlayStart_TWN_SEEK;\n    }"
   "    else if (overlayFileName == \"TWN_EXCA.PRG\")\n    {\n        gFieldOverlayFunction = overlayStart_TWN_EXCA;\n    }\n    else if (overlayFileName == \"TWN_E006.PRG\")\n    {\n        gFieldOverlayFunction = overlayStart_TWN_E006;\n    }"
   source_text "${source_text}")

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
   "                        if (offset)\n                        {\n                            sMatrix4x3 lagiWorldMatrix;\n                            initMatrixToIdentity(&lagiWorldMatrix);\n                            translateMatrix(pThis->mC_position, &lagiWorldMatrix);\n                            translateMatrix(readSaturnVec3(r14 + 4), &lagiWorldMatrix);\n\n                            sVec3_S16_12_4 lagiRotation;\n                            lagiRotation[0] = readSaturnS16(r14 + 0x10);\n                            lagiRotation[1] = readSaturnS16(r14 + 0x12);\n                            lagiRotation[2] = readSaturnS16(r14 + 0x14);\n                            rotateMatrixZYX_s16(lagiRotation, &lagiWorldMatrix);\n\n                            lagi::azel_bridge::SubmissionState lagiState{};\n                            for (unsigned int lagiRow = 0; lagiRow < 3; ++lagiRow)\n                            {\n                                for (unsigned int lagiCol = 0; lagiCol < 4; ++lagiCol)\n                                {\n                                    lagiState.modelMatrix[lagiRow * 4u + lagiCol] =\n                                        lagiWorldMatrix.m[lagiRow][lagiCol].asS32();\n                                }\n                            }\n                            lagiState.hasModelMatrix = true;\n                            lagiState.dynamic = false;\n                            lagi::azel_bridge::capture_current_light(lagiState);\n\n                            lagi::azel_bridge::set_town_submission_context(\n                                -1,\n                                static_cast<std::uint32_t>(pThis->m8_cellPtr.m_offset),\n                                static_cast<std::uint32_t>(r14.m_offset),\n                                static_cast<std::uint32_t>(offset),\n                                lagiState);\n\n                            addObjectToDrawList(pThis->m0_fileBundle->get3DModel(offset));\n                        }"
   source_text "${source_text}")
  endif()

  # Preserve Azel's title/menu ownership while supplying the Vita persistence
  # boundary and real slot availability. extern/Azel remains untouched.
  if(relative_path STREQUAL "titleScreen.cpp")
   string(REPLACE
    "#include \"o_title.h\""
    "#include \"o_title.h\"\n#include \"lagi/azel_save_platform.h\""
    source_text "${source_text}")
   string(REPLACE
    [=[bool hasSaveGame()
{
    PDS_unimplemented("hasSaveGame");
    return true;
}]=]
    [=[bool hasSaveGame()
{
    return lagiAzelSaveHasAnyValidSlot() != 0;
}]=]
    source_text "${source_text}")
   string(REPLACE
    "        pWorkArea->m_menu[0].m_isEnabled = (azelCdNumber == 0); // new game enabled only is first disk"
    "        pWorkArea->m_menu[0].m_isEnabled = (azelCdNumber == 0); // new game enabled only is first disk\n        pWorkArea->m_menu[1].m_isEnabled = hasSaveGame();"
    source_text "${source_text}")
  elseif(relative_path STREQUAL "kernel/loadSavegameScreen.cpp")
   string(REPLACE
    "#include \"3dEngine_textureCache.h\""
    "#include \"3dEngine_textureCache.h\"\n#include \"lagi/azel_save_platform.h\""
    source_text "${source_text}")
   # Saturn work areas are cleared before Init. The C++ task host uses plain
   # `new T`, so establish the load-overlay dispatch state explicitly before
   # its first Draw. Otherwise Continue may branch on an indeterminate status.
   string(REPLACE
    "    static void Init(sLoadSavegameScreen* pThis, p_workArea pParent)\n    {\n        pThis->m4 = pParent;"
    "    static void Init(sLoadSavegameScreen* pThis, p_workArea pParent)\n    {\n        // LAGI_LOAD_SCREEN_ZERO_INIT\n        pThis->m0_status = 0;\n        pThis->m4 = pParent;"
    source_text "${source_text}")
   # The native save/load task likewise expects its state and optional child
   # pointer to begin cleared. Preserve Azel's Init sequence after restoring
   # that original allocator contract at the Vita integration boundary.
   string(REPLACE
    "    static void Init(sSaveTask* pThis)\n    {\n        pThis->mCC = backupMemoryForSaveScreen();"
    "    static void Init(sSaveTask* pThis)\n    {\n        // LAGI_SAVE_TASK_ZERO_INIT\n        // Saturn work areas are cleared before Init. Initialize every scalar\n        // consumed by the native save/load pipeline before any child Draw.\n        pThis->m0 = 0;\n        pThis->m4_status = 0;\n        pThis->m8 = -1;\n        pThis->mC = 0;\n        pThis->m10 = 0;\n        pThis->m14_selectedDevice = -1;\n        pThis->m18 = nullptr;\n        pThis->m1C[0] = pThis->m1C[1] = pThis->m1C[2] = -1;\n        pThis->m20_selectedFileInDevice = -1;\n        pThis->m24 = nullptr;\n        pThis->m28_slotInfoDisplayTask[0] = nullptr;\n        pThis->m28_slotInfoDisplayTask[1] = nullptr;\n        pThis->m28_slotInfoDisplayTask[2] = nullptr;\n        pThis->m34_pSaveProgressTask = nullptr;\n        pThis->m38_operationResult = 0;\n        pThis->m3C[0].m0_slotStatus = 0;\n        pThis->m3C[1].m0_slotStatus = 0;\n        pThis->m3C[2].m0_slotStatus = 0;\n        pThis->mCC = backupMemoryForSaveScreen();"
    source_text "${source_text}")
   string(REPLACE
    "        pThis->mC = r5;\n        pThis->m20_stringRenderingX = intDivide(8, readSaturnS16(r5));"
    "        // LAGI_SAVE_SLOT_TASK_ZERO_INIT\n        pThis->m0 = -1;\n        pThis->m4_delay = 0;\n        pThis->m8 = nullptr;\n        pThis->mC = r5;\n        pThis->m10_status = 0;\n        pThis->m14 = 0;\n        pThis->m18 = 0;\n        pThis->m1C_posX = 0;\n        pThis->m1E_posY = 0;\n        pThis->m20_stringRenderingX = intDivide(8, readSaturnS16(r5));"
    source_text "${source_text}")
   string(REPLACE
    "    sSaveProgressTask* pNewTask = createSubTaskFromFunction<sSaveProgressTask>(parent, saveProgressTask_update);\n    pNewTask->m0 = &parent->m3C[parent->m20_selectedFileInDevice];"
    "    sSaveProgressTask* pNewTask = createSubTaskFromFunction<sSaveProgressTask>(parent, saveProgressTask_update);\n    // LAGI_SAVE_PROGRESS_ZERO_INIT\n    pNewTask->m10_status = 0;\n    pNewTask->m14 = 0;\n    pNewTask->m18 = 0;\n    pNewTask->m0 = &parent->m3C[parent->m20_selectedFileInDevice];"
    source_text "${source_text}")
   string(REPLACE
    [=[s32 writeSaveData(s32 deviceId, const std::string* filename, char* description, void* buffer, int size)
{
    char finalSaveFileName[1024];
    sprintf(finalSaveFileName, "save\\%d\\%s", deviceId, filename->c_str());
    FILE* fHandle = fopen(finalSaveFileName, "wb+");
    if (fHandle)
    {
        fwrite(buffer, size, 1, fHandle);
        fclose(fHandle);
        return 0;
    }
    return -1;
}]=]
    [=[s32 writeSaveData(s32 deviceId, const std::string* filename, char*, void* buffer, int size)
{
    return filename && buffer && size > 0
        ? lagiAzelSaveWrite(deviceId, filename->c_str(), buffer, static_cast<std::size_t>(size))
        : -1;
}]=]
    source_text "${source_text}")
   string(REPLACE
    [=[s32 readFrombackup(s32 deviceId, const std::string* filename, sSaveDataRaw* buffer)
{
    char finalSaveFileName[1024];
    sprintf(finalSaveFileName, "save\\%d\\%s", deviceId, filename->c_str());
    FILE* fHandle = fopen(finalSaveFileName, "rb");
    if (fHandle)
    {
        fread(buffer, sizeof(sSaveDataRaw), 1, fHandle);
        fclose(fHandle);
        return 0;
    }
    return -1;
}]=]
    [=[s32 readFrombackup(s32 deviceId, const std::string* filename, sSaveDataRaw* buffer)
{
    return filename && buffer
        ? lagiAzelSaveRead(deviceId, filename->c_str(), buffer, sizeof(sSaveDataRaw))
        : -1;
}]=]
    source_text "${source_text}")
   string(REPLACE
    [=[s32 getBupStatusWithBuffer(s32 slotIndex, u32 dataSize)
{
    return 1;
}

s32 getBupStatus(s32 slotIndex)
{
    return 0;
}]=]
    [=[s32 getBupStatusWithBuffer(s32 slotIndex, u32)
{
    return lagiAzelSaveFreeSlots(slotIndex);
}

s32 getBupStatus(s32 slotIndex)
{
    return lagiAzelSaveDeviceAvailable(slotIndex) ? 0 : -1;
}]=]
    source_text "${source_text}")
   string(REPLACE
    [=[        char finalSaveFileName[1024];
        sprintf(finalSaveFileName, "save\\%d\\%s", device, saveFileName.c_str());

        FILE* fHandle = fopen(finalSaveFileName, "rb");
        if (fHandle)
        {
            sSaveDataRaw* pNewGameState = new sSaveDataRaw;
            fread(pNewGameState, sizeof(sSaveDataRaw), 1, fHandle);
            fclose(fHandle);
            return pNewGameState;
        }

        return NULL;]=]
    [=[        sSaveDataRaw* pNewGameState = new sSaveDataRaw;
        if (lagiAzelSaveRead(device, saveFileName.c_str(),
                pNewGameState, sizeof(sSaveDataRaw)) == 0)
            return pNewGameState;
        delete pNewGameState;
        return NULL;]=]
    source_text "${source_text}")
   string(REPLACE
    "                assert(0); //060196c4 this would create the icon for the disabled slot"
    "                // Vita exposes only internal storage; unavailable Saturn backup devices stay disabled."
    source_text "${source_text}")
   string(REPLACE
    "                Unimplemented(); // TODO: setSoundOutputMode not yet implemented"
    "                // Audio output-mode restoration is deferred until that Saturn option is exposed."
    source_text "${source_text}")
   string(REPLACE
    "        Unimplemented();\n    }\n\n    // 06019bd4"
    "    }\n\n    // 06019bd4"
    source_text "${source_text}")
   string(REGEX MATCHALL
    "LAGI_(LOAD_SCREEN|SAVE_TASK|SAVE_SLOT_TASK|SAVE_PROGRESS)_ZERO_INIT"
    lagi_save_zero_init_matches
    "${source_text}")
   list(LENGTH lagi_save_zero_init_matches lagi_save_zero_init_match_count)
   if(NOT lagi_save_zero_init_match_count EQUAL 4)
    message(FATAL_ERROR
     "Save/load zero-init patches did not apply exactly four times: found "
     "${lagi_save_zero_init_match_count} matches")
   endif()
  elseif(relative_path STREQUAL "kernel/menuCursor.cpp")
   string(REPLACE
    "    pThis->m4 = pMenuData->m0;\n    pThis->m8 = pMenuData->m4;"
    "    // LAGI_MENU_CURSOR_ZERO_INIT\n    pThis->m0_selectedEntry = -1;\n    pThis->m4 = pMenuData->m0;\n    pThis->m8 = pMenuData->m4;\n    pThis->mC = 0;"
    source_text "${source_text}")
   string(REPLACE
    "    pThis->m4 = config;\n    pThis->mC = 2;"
    "    // LAGI_MENU_CURSOR2_ZERO_INIT\n    pThis->m0 = -1;\n    pThis->m4 = config;\n    pThis->m8 = 0;\n    pThis->mC = 2;"
    source_text "${source_text}")
   string(REGEX MATCHALL
    "LAGI_MENU_CURSOR2?_ZERO_INIT"
    lagi_menu_cursor_zero_init_matches
    "${source_text}")
   list(LENGTH lagi_menu_cursor_zero_init_matches lagi_menu_cursor_zero_init_match_count)
   if(NOT lagi_menu_cursor_zero_init_match_count EQUAL 2)
    message(FATAL_ERROR
     "Menu cursor zero-init patches did not apply exactly twice: found "
     "${lagi_menu_cursor_zero_init_match_count} matches")
   endif()
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
 rootTask.cpp
 titleScreen.cpp
 o_title.cpp
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
 menu/inventoryMenu.cpp
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
 movie/movie.cpp
 kernel/loadSavegameScreen.cpp
 kernel/textDisplay.cpp
 items.cpp
 menu_dragon.cpp
 field.cpp
 field/fieldEncounterTask.cpp
 field/fieldCamera.cpp
 field/fieldDragon.cpp
 field/fieldVisibilityGrid.cpp
 field/fieldRadar.cpp
 field/fieldCutsceneTask.cpp
 field/fieldCutsceneTask2.cpp
 field/fieldCutsceneTask3.cpp
 field/exitField.cpp
 field/fieldItemBoxDefinition.cpp
 field/fieldItemBox.cpp
 field/fieldItemBox_type0.cpp
 field/fieldItemBox_type1.cpp
 field/fieldItemBox_type2.cpp
 field/fieldSavePoint.cpp
 field/fieldParticlePool.cpp
 field/fieldDebrisScatter.cpp
 field/battleStart.cpp
 field/fieldDragonMovement.cpp
 field/fieldDragonInput.cpp
 field/fieldDialog.cpp
 field/fieldScriptWaitTask.cpp
 field/dragonLightWingEvolution.cpp
 field/field_a3/a3_0_task4.cpp
 field/field_a3/a3_2_crashedImperialShip.cpp
 field/field_a3/a3_background_layer.cpp
 field/field_a3/a3_birds.cpp
 field/field_a3/a3_crane_lower.cpp
 field/field_a3/a3_crane_upper.cpp
 field/field_a3/a3_dynamic_mine_cart.cpp
 field/field_a3/a3_fan.cpp
 field/field_a3/a3_static_mine_cart.cpp
 field/field_a3/a3_waterfall.cpp
 field/field_a3/o_fld_a3.cpp
 field/field_a3/o_fld_a3_0.cpp
 field/field_a3/o_fld_a3_1.cpp
 field/field_a3/o_fld_a3_2.cpp
 field/field_a3/o_fld_a3_3.cpp
 field/field_a3/o_fld_a3_4.cpp
 field/field_a3/o_fld_a3_5.cpp
 field/field_a3/o_fld_a3_6.cpp
 field/field_a3/o_fld_a3_7.cpp
 field/field_a3/o_fld_a3_8.cpp
 field/field_a3/o_fld_a3_9.cpp
 field/field_a3/o_fld_a3_A.cpp
 field/field_a3/o_fld_a3_B.cpp
 field/field_a3/o_fld_a3_C.cpp
 LCS.cpp
 field/field_d5/o_fld_d5.cpp
 field/field_d5/d5_nameEntry.cpp
 field/field_d5/d5_particles.cpp
 field/field_d5/d5_starfield.cpp
 field/field_d5/d5_gameOver.cpp
 dragonMorphing.cpp
 dragonRider.cpp
 battle/battleManager.cpp
 battle/battleDebugList.cpp
 battle/battleOverlay.cpp
 battle/battlePowerGauge.cpp
 battle/gunShotRootTask.cpp
 kernel/menuSprite.cpp
 kernel/menuCursor.cpp
 kernel/flagEditor.cpp
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
 # E006 is Azel's native destination after the first FLD_A3 flight segment.
 # Its VDP2 background task is shared with the upstream excavation overlay.
 town/exca/twn_exca.cpp
 town/excaEntity.cpp
 town/townDragon.cpp
 town/e006/twn_e006.cpp
 town/ruin/twn_ruin.cpp
 town/ruin/twn_ruin_lock.cpp
)

set(LAGI_AZEL_UPSTREAM_SOURCES)
foreach(relative_source IN LISTS LAGI_AZEL_TOWN_RUNTIME_RELATIVE_SOURCES)
 lagi_prepare_upstream_azel_source("${relative_source}" generated_source)
 list(APPEND LAGI_AZEL_UPSTREAM_SOURCES "${generated_source}")
endforeach()

# DSP native corpus is optional. Empty corpus keeps the interpreter available.
set(LAGI_DSP_CORPUS "${CMAKE_SOURCE_DIR}/dsp_programs" CACHE PATH "Local LDSP capture directory")
find_program(LAGI_DSP_PYTHON NAMES python python3 HINTS "$ENV{USERPROFILE}/.cache/codex-runtimes/codex-primary-runtime/dependencies/python")
if(NOT LAGI_DSP_PYTHON)
 message(FATAL_ERROR "DSP generation needs Python 3; set LAGI_DSP_PYTHON to its executable")
endif()
file(GLOB_RECURSE LAGI_DSP_CAPTURES CONFIGURE_DEPENDS "${LAGI_DSP_CORPUS}/*.ldsp")
set(LAGI_DSP_AOT "${CMAKE_BINARY_DIR}/lagi_dsp_aot.inc")
add_custom_command(OUTPUT "${LAGI_DSP_AOT}"
 COMMAND "${LAGI_DSP_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/generate_scspdsp_aot.py"
  --corpus "${LAGI_DSP_CORPUS}" --output "${LAGI_DSP_AOT}"
 DEPENDS "${CMAKE_SOURCE_DIR}/tools/generate_scspdsp_aot.py"
  "${CMAKE_SOURCE_DIR}/src/integration/lagi_dsp_fields.def" ${LAGI_DSP_CAPTURES}
 VERBATIM)
file(READ "${CMAKE_SOURCE_DIR}/extern/Azel/ThirdParty/aosdk/eng_ssf/scsp.c" lagi_scsp_source)
set(lagi_dsp_init_anchor "memset(SCSP,0,sizeof(*SCSP));")
string(FIND "${lagi_scsp_source}" "${lagi_dsp_init_anchor}" lagi_dsp_init_match)
string(FIND "${lagi_scsp_source}" "${lagi_dsp_init_anchor}" lagi_dsp_init_last REVERSE)
if(lagi_dsp_init_match LESS 0 OR NOT lagi_dsp_init_match EQUAL lagi_dsp_init_last)
 message(FATAL_ERROR "SCSP DSP initialization anchor changed")
endif()
string(REPLACE
 "${lagi_dsp_init_anchor}"
 "${lagi_dsp_init_anchor}\n\t\tSCSPDSP_Init(&SCSP->DSP);"
 lagi_scsp_source "${lagi_scsp_source}")
set(lagi_mpro_anchor "*((unsigned short *) (SCSP->DSP.MPRO+(addr-0x800)/2))=val;")
string(FIND "${lagi_scsp_source}" "${lagi_mpro_anchor}" lagi_mpro_match)
string(FIND "${lagi_scsp_source}" "${lagi_mpro_anchor}" lagi_mpro_last REVERSE)
if(lagi_mpro_match LESS 0 OR NOT lagi_mpro_match EQUAL lagi_mpro_last)
 message(FATAL_ERROR "SCSP MPRO write hook anchor changed")
endif()
string(REPLACE "${lagi_mpro_anchor}" "{ lagi_dsp_program_written(&SCSP->DSP); ${lagi_mpro_anchor} }" lagi_scsp_source "${lagi_scsp_source}")
set(LAGI_SCSP_SERVICE "${CMAKE_BINARY_DIR}/lagi_scsp_service.c")
file(WRITE "${LAGI_SCSP_SERVICE}" "#include \"lagi_dsp_platform.h\"\n${lagi_scsp_source}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/extern/Azel/ThirdParty/aosdk/eng_ssf/scsp.c")

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
 ${LAGI_CINEPAK_F_OBJ}
 ${LAGI_VDP2_NBG_F_OBJ}
 ${LAGI_VDP2_RBG0_F_OBJ}
 src/platform/vita/vita_audio.cpp
 extern/Azel/ThirdParty/aosdk/eng_ssf/m68kcpu.c
 extern/Azel/ThirdParty/aosdk/eng_ssf/m68kopac.c
 extern/Azel/ThirdParty/aosdk/eng_ssf/m68kopdm.c
 extern/Azel/ThirdParty/aosdk/eng_ssf/m68kopnz.c
 extern/Azel/ThirdParty/aosdk/eng_ssf/m68kops.c
 extern/Azel/ThirdParty/aosdk/eng_ssf/sat_hw.c
 ${LAGI_SCSP_SERVICE}
 ${LAGI_DSP_AOT}
 src/integration/lagi_scspdsp_predecoded.c
 src/integration/lagi_dsp_platform_vita.cpp
 src/movie/film_demuxer.cpp
 src/movie/cinepak_decoder.cpp
 src/movie/cinepak_sgx_decoder.cpp
 src/movie/movie_player.cpp
 src/integration/azel_movie_bridge.cpp
 src/integration/lagi_azel_movie_runtime.cpp
 src/integration/lagi_runtime.cpp
 src/integration/lagi_input_bridge.cpp
 src/integration/lagi_scene_bridge.cpp
 src/integration/lagi_diagnostics.cpp
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
 src/integration/lagi_save_platform_vita.cpp
 src/integration/lagi_system_menu_vita.cpp
 src/integration/lagi_common_overlay_vita.cpp
 src/integration/lagi_sound_table_vita.cpp
 src/integration/lagi_dragon_common_vita.cpp
 src/integration/lagi_hotpoint_vita.cpp
 src/integration/lagi_audio_vita.cpp
 src/integration/lagi_renderer_host_vita.cpp
 src/integration/lagi_vdp1_animated_quad_vita.cpp
 ${LAGI_AZEL_UPSTREAM_SOURCES}
)

# Neptune performs the measured field transforms, lighting, material decoding
# and submission preparation. Bring-up configurations often have no build type,
# so make its optimization explicit rather than depending on Release flags.
# Keep floating-point operation order; hardware A/B can disable this separately.
option(LAGI_OPTIMIZE_NEPTUNE "Optimize Neptune CPU renderer" ON)
if(LAGI_OPTIMIZE_NEPTUNE)
 set_source_files_properties(
  src/platform/vita/neptune_renderer.cpp
  PROPERTIES COMPILE_OPTIONS "-O2;-fno-fast-math;-ffp-contract=off"
             COMPILE_DEFINITIONS "LAGI_NEPTUNE_OPTIMIZED=1"
 )
else()
 set_source_files_properties(
  src/platform/vita/neptune_renderer.cpp
  PROPERTIES COMPILE_DEFINITIONS "LAGI_NEPTUNE_OPTIMIZED=0"
 )
endif()

# The SCSP DSP interpreter is the measured native-audio hotspot. Optimize only
# the Lagi-owned replacement; keep the rest of the bring-up build unchanged.
set_source_files_properties(
 src/integration/lagi_scspdsp_predecoded.c
 PROPERTIES COMPILE_OPTIONS "-O2"
)

# Optimize the native SCSP slot mixer and hardware service independently from
# the DSP interpreter so each audio-stage gain remains hardware-testable.
set_source_files_properties(
 extern/Azel/ThirdParty/aosdk/eng_ssf/sat_hw.c
 ${LAGI_SCSP_SERVICE}
 PROPERTIES COMPILE_OPTIONS "-O2"
)

# The 68K service consumes most of the remaining per-quantum budget. Keep its
# optimization isolated from both the SCSP checkpoint and the rest of Lagi.
set_source_files_properties(
 extern/Azel/ThirdParty/aosdk/eng_ssf/m68kcpu.c
 extern/Azel/ThirdParty/aosdk/eng_ssf/m68kopac.c
 extern/Azel/ThirdParty/aosdk/eng_ssf/m68kopdm.c
 extern/Azel/ThirdParty/aosdk/eng_ssf/m68kopnz.c
 extern/Azel/ThirdParty/aosdk/eng_ssf/m68kops.c
 PROPERTIES COMPILE_OPTIONS "-O2"
)

target_include_directories(lagi PRIVATE include
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/town"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/town/ruin"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/town/exca"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/town/e006"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/kernel"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/audio"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/ThirdParty/aosdk"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/ThirdParty/aosdk/eng_ssf"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/shared"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/field"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/field/field_a3"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/field/field_d5"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/battle"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/battle/BTL_A3"
 "${CMAKE_CURRENT_SOURCE_DIR}/extern/Azel/AzelLib/menu"
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

# The converter appends SCE metadata to the executable segment. Default page
# alignment can leave too little space before data for some DSP corpus sizes.
target_link_options(lagi PRIVATE "-Wl,-T,${CMAKE_SOURCE_DIR}/tools/vita_sce_gap.ld")
set_property(TARGET lagi APPEND PROPERTY LINK_DEPENDS "${CMAKE_SOURCE_DIR}/tools/vita_sce_gap.ld")
target_include_directories(lagi PRIVATE "${CMAKE_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/src/integration")
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
