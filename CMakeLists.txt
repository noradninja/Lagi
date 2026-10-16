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

option(LAGI_STOCHASTIC_FILTER "Use SGX stochastic filter for UI and Cinepak" ON)
option(LAGI_DITHER_LOOKUP "Use a repeating FP16 offset texture instead of screen parity arithmetic" ON)
option(LAGI_FULLRES "Render native Vita gameplay/movie modes at 960x544 (test only)" OFF)
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
set(LAGI_STOCHASTIC_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_stochastic_f.gxp")
set(LAGI_STOCHASTIC_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_stochastic_f_gxp.o")
set(LAGI_STOCHASTIC_GOURAUD_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_stochastic_gouraud_f.gxp")
set(LAGI_STOCHASTIC_GOURAUD_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_stochastic_gouraud_f_gxp.o")
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
 OUTPUT "${LAGI_STOCHASTIC_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}" -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/stochastic_f.cg" -DLAGI_DITHER_LOOKUP=$<BOOL:${LAGI_DITHER_LOOKUP}> -Wperf -fastprecision -O3 -o "${LAGI_STOCHASTIC_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/stochastic_f.cg"
 VERBATIM
)
add_custom_command(
 OUTPUT "${LAGI_STOCHASTIC_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}" -I binary -O elf32-littlearm -B arm "lagi_stochastic_f.gxp" "${LAGI_STOCHASTIC_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_STOCHASTIC_F_GXP}"
 VERBATIM
)
add_custom_command(
 OUTPUT "${LAGI_STOCHASTIC_GOURAUD_F_GXP}"
 COMMAND "${PSP2CGC_EXECUTABLE}" -profile sce_fp_psp2
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/stochastic_gouraud_f.cg" -DLAGI_DITHER_LOOKUP=$<BOOL:${LAGI_DITHER_LOOKUP}> -Wperf -fastprecision -O3 -o "${LAGI_STOCHASTIC_GOURAUD_F_GXP}"
 DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/stochastic_gouraud_f.cg"
 VERBATIM
)
add_custom_command(
 OUTPUT "${LAGI_STOCHASTIC_GOURAUD_F_OBJ}"
 COMMAND "${CMAKE_OBJCOPY}" -I binary -O elf32-littlearm -B arm "lagi_stochastic_gouraud_f.gxp" "${LAGI_STOCHASTIC_GOURAUD_F_OBJ}"
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 DEPENDS "${LAGI_STOCHASTIC_GOURAUD_F_GXP}"
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
 "${LAGI_TEXTURE_V_OBJ}" "${LAGI_TEXTURE_F_OBJ}" "${LAGI_STOCHASTIC_F_OBJ}"
 "${LAGI_STOCHASTIC_GOURAUD_F_OBJ}"
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
 if(relative_path STREQUAL "mainMenuDebugTasks.cpp")
  string(REPLACE
   "void s_FieldSubTaskWorkArea::Init(s_FieldSubTaskWorkArea* pFieldSubTaskWorkArea)\n{"
   "void s_FieldSubTaskWorkArea::Init(s_FieldSubTaskWorkArea* pFieldSubTaskWorkArea)\n{\n    pFieldSubTaskWorkArea->fieldSubTaskStatus = 0; // LAGI_FIELD_ENTRY_ZERO_STATE"
   source_text "${source_text}")
  string(REPLACE
   "            if (!isSoundLoadingFinished())\n            {\n                g_fadeControls.m_4D = 6;"
   "            if (isSoundLoadingFinished()) // LAGI_FIELD_ENTRY_AUDIO_READY: Vita completion is true\n            {\n                g_fadeControls.m_4D = 6;"
   source_text "${source_text}")
  if(NOT source_text MATCHES "LAGI_FIELD_ENTRY_ZERO_STATE" OR NOT source_text MATCHES "LAGI_FIELD_ENTRY_AUDIO_READY")
   message(FATAL_ERROR "Native field entry anchors changed")
  endif()
 endif()
 if(relative_path STREQUAL "field/fieldRadar.cpp")
  set(radar_destination_old [=[static void fieldRadar_drawDestinationIndicator(s_FieldRadar* pThis, s32* pDestPosition)
{
    Unimplemented();
}]=])
  set(radar_destination_new [=[static void fieldRadar_drawDestinationIndicator(s_FieldRadar* pThis, s32* pDestPosition)
{
    lagiDrawFieldRadarDestination(pThis, pDestPosition); // LAGI_NATIVE_RADAR_DESTINATION: 06071EC4
}]=])
  string(FIND "${source_text}" "${radar_destination_old}" radar_destination_match)
  if(radar_destination_match LESS 0)
   message(FATAL_ERROR "Native radar destination anchor changed")
  endif()
  string(REPLACE "${radar_destination_old}" "${radar_destination_new}" source_text "${source_text}")
  # FLD_A3 06071E3E initializer: 06071E7C, 06071E76/8C, 06071E8E/98.
  # Preserve the native 24 -> 10 direction segment and positive center offset.
  foreach(radar_init_pair IN ITEMS
    "mC_X = pTypedWorkArea->m8 + 0x30|mC_X = pTypedWorkArea->m8 + 0x60"
    "m34 = 0x18|m34 = 0x0A"
    "m28 = -0x1D|m28 = 0x1D")
   string(REPLACE "|" ";" radar_init_parts "${radar_init_pair}")
   list(GET radar_init_parts 0 radar_init_old)
   list(GET radar_init_parts 1 radar_init_new)
   string(FIND "${source_text}" "${radar_init_old}" radar_init_match)
   if(radar_init_match LESS 0)
    message(FATAL_ERROR "Native radar initializer anchor changed: ${radar_init_old}")
   endif()
   string(REPLACE "${radar_init_old}" "${radar_init_new}" source_text "${source_text}")
  endforeach()
  set(radar_begin "void fieldRadar_draw(s_FieldRadar* pThis)\n{")
  set(radar_end "    // Frame counter\n    pThis->m44++;")
  foreach(anchor IN ITEMS radar_begin radar_end)
   string(FIND "${source_text}" "${${anchor}}" radar_match)
   if(radar_match LESS 0)
    message(FATAL_ERROR "Native radar command ownership anchor changed: ${anchor}")
   endif()
  endforeach()
  string(REPLACE "${radar_begin}" "${radar_begin}\n    const auto lagiRadarBegin = graphicEngineStatus.m14_vdp1Context[0].m0_currentVdp1WriteEA;" source_text "${source_text}")
  string(REPLACE "${radar_end}" "    lagiMarkFieldRadarCommands(&*lagiRadarBegin, &*graphicEngineStatus.m14_vdp1Context[0].m0_currentVdp1WriteEA);\n${radar_end}" source_text "${source_text}")
 endif()
 if(relative_path STREQUAL "LCS.cpp")
  # 0607BD9A stores the orbit angle in the sparkle task (+10), NOT
  # dragon +EC. The local rotation already services its only consumer here.
  string(FIND "${source_text}" "pDragon->m_EC = rotation;" sparkle_flags_anchor)
  if(sparkle_flags_anchor LESS 0)
   message(FATAL_ERROR "Native sparkle lighting-control anchor changed")
  endif()
  string(REPLACE "pDragon->m_EC = rotation;"
   "// LAGI_SPARKLE_FLAGS_PRESERVED: orbit angle is not dragon lighting flags."
   source_text "${source_text}")
  set(collection_offset_old "pThis->m2C_localOffset[2] = 0;")
  set(collection_offset_new "pThis->m2C_localOffset[2] = pThis->m34_randomizedLifetime; // LAGI_NATIVE_COLLECTION_RADIUS: 0607BAE4")
  set(collection_draw_old [=[        // TODO: FUN_0602f610 — draw billboard sprite at cameraSpacePos with scaledOpacity
        Unimplemented();]=])
  set(collection_draw_new [=[        // LAGI_NATIVE_COLLECTION_BURST: 0607BC06..52 -> 0602F610.
        std::array<sVec3_FP, 2> radial = {*pThis->m8_pPosition, cameraSpacePos};
        std::array<fixedPoint, 2> widths = {0, scaledOpacity};
        const u16 base = (pThis->m0_memoryArea.m4_characterArea - 0x25C00000) >> 3;
        quadColor colors;
        const auto colorData = gFLD_A3->getSaturnPtr(0x06094F64) + pThis->m1E_colorIndex * 8;
        for (unsigned corner = 0; corner < 4; ++corner)
            colors[corner] = readSaturnU16(colorData + corner * 2);
        displayRaySegment_2Width(radial, widths, base + 0x6B0, 0x031E, base + 0x1780, &colors, 8);]=])
  foreach(anchor IN ITEMS collection_offset_old collection_draw_old)
   string(FIND "${source_text}" "${${anchor}}" collection_match)
   if(collection_match LESS 0)
    message(FATAL_ERROR "Native collection burst anchor changed: ${anchor}")
   endif()
  endforeach()
  string(REPLACE "${collection_offset_old}" "${collection_offset_new}" source_text "${source_text}")
  string(REPLACE "${collection_draw_old}" "${collection_draw_new}" source_text "${source_text}")
  string(REPLACE "#include \"PDS.h\"" "#include \"PDS.h\"\n#include \"kernel/rayDisplay.h\"" source_text "${source_text}")
  set(radar_objects_anchor "void LCSTaskDrawSub5(s_LCSTask* r4)\n{")
  string(FIND "${source_text}" "${radar_objects_anchor}" radar_objects_match)
  if(radar_objects_match LESS 0)
   message(FATAL_ERROR "Native LCS radar marker anchor changed")
  endif()
  string(REPLACE "${radar_objects_anchor}"
   "${radar_objects_anchor}\n    lagiDrawFieldRadarObjectMarkers(r4); // LAGI_NATIVE_RADAR_OBJECTS: 0606E750"
   source_text "${source_text}")
 endif()
 if(relative_path STREQUAL "kernel/dialogTask.cpp")
  set(navigator_cancel_old "*pThis->mC_result = pThis->m2_defaultResult - 1;")
  set(navigator_cancel_new "*pThis->mC_result = -1; // LAGI_NATIVE_DIALOG_CANCEL: 0601DCA2/6")
  set(navigator_delete_old "void s_multiChoiceTask2_Delete(s_multiChoiceTask* pThis)\n{\n    clearMultiChoiceBox(pThis);")
  set(navigator_delete_new "${navigator_delete_old}\n    lagiReleaseFieldNavigatorDialog(pThis);")
  foreach(anchor IN ITEMS navigator_cancel_old navigator_delete_old)
   string(FIND "${source_text}" "${${anchor}}" navigator_dialog_match)
   if(navigator_dialog_match LESS 0)
    message(FATAL_ERROR "Native navigator dialog anchor changed: ${anchor}")
   endif()
  endforeach()
  string(REPLACE "${navigator_cancel_old}" "${navigator_cancel_new}" source_text "${source_text}")
  string(REPLACE "${navigator_delete_old}" "${navigator_delete_new}" source_text "${source_text}")
 endif()
 string(REPLACE
  "#include \"PDS.h\""
  "#include \"lagi/lagi_azel_upstream_prelude.h\""
  source_text
  "${source_text}"
 )

 # Native map Update (0607CDC2): +20 is target +18, and the final
 # visibility test reads byte +8D, not the received-item word +88.
 # Scope this to maps: aliasing normal-box callback status changed its
 # early-return behavior and was rejected by hardware testing.
 if(relative_path STREQUAL "field/fieldItemBox_type1.cpp")
  string(FIND "${source_text}" "(pThis->m88_poseIdx == 0)" map_visibility_anchor)
  if(map_visibility_anchor LESS 0)
   message(FATAL_ERROR "Native map visibility anchor changed")
  endif()
  string(REPLACE "pThis->m20" "pThis->m8_LCSTarget.m18_diableFlags" source_text "${source_text}")
  string(REPLACE "(pThis->m88_poseIdx == 0)" "(pThis->m8D_visibilityFlag != 0)" source_text "${source_text}")
 endif()

 # The shared particle task serves both map pickups and save stations.
 # Maps emit only before their accepted collection transition; station
 # activation retains its existing nonzero-state gate.
 if(relative_path STREQUAL "field/fieldItemBox_type2.cpp")
  string(REGEX MATCHALL "if \\(pParent->mEA_state == 0\\)" map_particle_gates "${source_text}")
  list(LENGTH map_particle_gates map_particle_gate_count)
  if(NOT map_particle_gate_count EQUAL 2)
   message(FATAL_ERROR "Shared map/station particle Update/Draw gates changed")
  endif()
  string(REPLACE "if (pParent->mEA_state == 0)"
   "if (pParent->m8B_LCSType == 1 ? pParent->mEA_state != 0 : pParent->mEA_state == 0) // LAGI_MAP_PARTICLE_GATE"
   source_text "${source_text}")
 endif()

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
  # Native 06025EB8 uses column-major ownership (x*64+y), unlike row-major
  # VRAM. 06025F06/0E normalize both coordinates to the glyph origin.
  set(glyph_clear_read "    u16 var0 = characterMap2[(y << 6) + x];")
  set(glyph_clear_old [=[    u16* var_14 = characterMap2 + ((x - (r13 & 1)) << 7);
    y -= (2 & r13) >> 1;
    var0 = (y << 1);
    *(var_14 + (var0 >> 1)) = 0;

    u16* var_10 = characterMap2 + ((x + 1) << 7);
    *(var_10 + (var0 >> 1)) = 0;
    *(var_14 + (y + 1)) = 0;

    *(var_10 + (y + 1)) = 0;]=])
  set(glyph_clear_new [=[    // LAGI_NATIVE_GLYPH_CLEAR: 06025F00..06025F6E.
    x -= r13 & 1;
    y -= (r13 & 2) >> 1;
    characterMap2[x * 64 + y] = 0;
    characterMap2[(x + 1) * 64 + y] = 0;
    characterMap2[x * 64 + y + 1] = 0;
    characterMap2[(x + 1) * 64 + y + 1] = 0;]=])
  foreach(glyph_clear_anchor IN ITEMS glyph_clear_read glyph_clear_old)
   string(FIND "${source_text}" "${${glyph_clear_anchor}}" glyph_clear_match)
   if(glyph_clear_match LESS 0)
    message(FATAL_ERROR "Native glyph-clear anchor changed: ${glyph_clear_anchor}")
   endif()
  endforeach()
  string(REPLACE "${glyph_clear_read}" "    u16 var0 = characterMap2[x * 64 + y];" source_text "${source_text}")
  string(REPLACE "${glyph_clear_old}" "${glyph_clear_new}" source_text "${source_text}")
  # 06025780 identifies context base 060525D0; +8 is global string table.
  set(global_substitution_old [=[void printVdp2StringSub2(s32 r4)
{
    Unimplemented();
}]=])
  set(global_substitution_new [=[void printVdp2String(s_stringStatusQuery* vars);
void printVdp2StringSub2(s_stringStatusQuery* vars, s32 index)
{
    if (index < 0 || index >= vdp2StringContext2.m0_numEntries ||
        !vdp2StringContext2.m8_globalStrings) return;
    const char* text = vdp2StringContext2.m8_globalStrings[index];
    if (!text || !*text) return;
    const char* outer = vars->m20_string;
    const auto alignment = vars->m2C;
    vars->m20_string = text;
    // Substitute at the live cursor; do not center the inserted fragment again.
    vars->m2C &= ~1;
    printVdp2String(vars); // LAGI_NATIVE_GLOBAL_STRING_SUBSTITUTION
    vars->m20_string = outer;
    vars->m2C = alignment;
}]=])
  string(FIND "${source_text}" "${global_substitution_old}" global_substitution_offset)
  if(global_substitution_offset LESS 0)
   message(FATAL_ERROR "Native global-string substitution adapter missing")
  endif()
  string(REPLACE "${global_substitution_old}" "${global_substitution_new}" source_text "${source_text}")
  string(REPLACE "printVdp2StringSub2(*(vars->m20_string++) - 0x30);"
   "printVdp2StringSub2(vars, *(vars->m20_string++) - 0x30);\n                r11 = (printVdp2StringTable[vars->m28] << 12) + 0x63;"
   source_text "${source_text}")
  string(APPEND source_text [=[

bool lagiGlobalStringTableEntryHasText(int index)
{
    if (index < 0 || index >= vdp2StringContext2.m0_numEntries ||
        !vdp2StringContext2.m8_globalStrings) return false;
    const char* text = vdp2StringContext2.m8_globalStrings[index];
    return text && static_cast<unsigned char>(text[0]) != 0;
}
]=])
  # Native 06024FB0 links the old tail to the newly loaded font. Without
  # that link, font indices are reused and setActiveFont cannot find them.
  set(font_link_old "    pNew->pPrevious = pOld;\n    pNew->pNext = NULL;\n    pNew->m14 = pData[1];")
  set(font_link_new "    pNew->pPrevious = pOld;\n    pNew->pNext = NULL;\n    pNew->m14 = pData[1];\n    pNew->m15 = 0; // Native 06024F88: no ownership flags on borrowed font data.\n    pOld->pNext = pNew; // LAGI_NATIVE_FONT_LINK: native 06024FB0.")
  string(FIND "${source_text}" "${font_link_old}" font_link_position)
  if(font_link_position LESS 0)
   message(FATAL_ERROR "Native VDP2 font-list adapter target changed")
  endif()
  string(REPLACE "${font_link_old}" "${font_link_new}" source_text "${source_text}")
  set(font_rect_old "    PDS_unimplemented(\"clearVdp2TextMemoryRect\");")
  set(font_rect_new "    // LAGI_NATIVE_FONT_RECT: native 06025166, 64 cells/row, 16-bit cells.\n    if (r6 <= 0 || r7 <= 0) return;\n    for (s32 row = 0; row < r7; ++row)\n        memset(getVdp2Vram(0x6000 + ((r5 + row) * 64 + r4) * 2), 0, r6 * 2);")
  string(FIND "${source_text}" "${font_rect_old}" font_rect_position)
  if(font_rect_position LESS 0)
   message(FATAL_ERROR "Native font rectangle-clear target changed")
  endif()
  string(REPLACE "${font_rect_old}" "${font_rect_new}" source_text "${source_text}")
  set(font_bottom_left_old "                characterMap2[vars->m0_cursorX * 0x40 + vars->m4_cursorY] = characterSprite + 2;")
  set(font_bottom_left_new "                characterMap2[vars->m0_cursorX * 0x40 + vars->m4_cursorY + 1] = characterSprite + 2; // LAGI_NATIVE_FONT_BOTTOM_LEFT: 06025DA0.")
  string(FIND "${source_text}" "${font_bottom_left_old}" font_bottom_left_position)
  if(font_bottom_left_position LESS 0)
   message(FATAL_ERROR "Native font bottom-left map target changed")
  endif()
  string(REPLACE "${font_bottom_left_old}" "${font_bottom_left_new}" source_text "${source_text}")
  string(REPLACE
   "void printVdp2Number3(s_stringStatusQuery* vars, int value, int length)\n{\n    Unimplemented();\n}"
   [=[int loadExtendedCharacter(u16 character);

void printVdp2Number3(s_stringStatusQuery* vars, int value, int length)
{
    // LAGI_NATIVE_NUMBER_PRINT: native 060258C0..06025AC4.
    u16 digits[11]{};
    int first = 11;
    u32 magnitude = static_cast<u32>(value);
    if (value < 0) magnitude = 0u - magnitude;
    do
    {
        digits[--first] = static_cast<u16>(0x8002 + magnitude % 10);
        magnitude /= 10;
    } while (magnitude);
    if (value < 0) digits[--first] = 0x8041;
    // Native storage holds eleven characters; callers request widths 2/3.
    if (length > 11) length = 11;
    while (length > 0 && 11 - first < length) digits[--first] = 0;
    const u32 palette = printVdp2StringTable[vars->m28] * 0x1000 + 0x400;
    for (int i = first; i < 11; ++i)
    {
        const u16 glyph = static_cast<u16>(loadExtendedCharacter(digits[i] ? digits[i] : 0x8001));
        const u16 tile = static_cast<u16>(palette + glyph * 4);
        const u32 offset = vars->m24_vdp2MemoryOffset;
        setVdp2VramU16(offset, tile);
        setVdp2VramU16(offset + 2, tile + 1);
        setVdp2VramU16(offset + 0x80, tile + 2);
        setVdp2VramU16(offset + 0x82, tile + 3);
        const int x = vars->m0_cursorX;
        const int y = vars->m4_cursorY;
        characterMap2[x * 64 + y] = tile;
        characterMap2[(x + 1) * 64 + y] = tile + 1;
        characterMap2[x * 64 + y + 1] = tile + 2;
        characterMap2[(x + 1) * 64 + y + 1] = tile + 3;
        vars->m24_vdp2MemoryOffset += 4;
        vars->m0_cursorX += 2;
    }
}]=]
   source_text "${source_text}")
  if(NOT source_text MATCHES "LAGI_NATIVE_NUMBER_PRINT")
   message(FATAL_ERROR "Native VDP2 number printer target changed")
  endif()
  string(REPLACE
   "s32 resetVdp2StringsSub1(u16* pData)"
   [=[// LAGI_NATIVE_FONT_UNLOAD: 0602502C, 060250E0 and 060250E6.
static void lagiAzelReleaseFont(sVdp2StringControl* font)
{
    if (!font || !font->m8) return;
    if (font->m15 & 1) freeHeap(font->m8);
    if (font->pPrevious)
    {
        if (pVdp2StringControl == font)
        {
            pVdp2StringControl = font->pPrevious;
            font->pPrevious->pNext = font->pNext;
        }
        else
        {
            if (font->pNext) font->pNext->pPrevious = font->pPrevious;
            font->pPrevious->pNext = font->pNext;
        }
        freeHeap(font);
    }
    resetCharacterMaps();
}

void lagiAzelUnloadCurrentFont()
{
    lagiAzelReleaseFont(pVdp2StringControl);
}

void lagiAzelUnloadFontByIndex(u32 index)
{
    auto* font = &vdp2StringControlBuffer;
    while (font && font->m0_index != index) font = font->pNext;
    lagiAzelReleaseFont(font);
}

s32 resetVdp2StringsSub1(u16* pData)]=]
   source_text "${source_text}")
  # Azel owns a 512 KiB vdp2Ram buffer. Its reconstructed 2 MiB mask can
  # return pointers outside that allocation; wrap to the actual VRAM size.
  string(REPLACE
   "return vdp2Ram + (offset & 0x1FFFFF);"
   "return vdp2Ram + (offset & (sizeof(vdp2Ram) - 1u));"
   source_text "${source_text}")
  if(NOT source_text MATCHES "sizeof\\(vdp2Ram\\) - 1u")
   message(FATAL_ERROR "VDP2 VRAM address adapter did not apply")
  endif()
  string(REPLACE
   "        case m45_COEN:\n            vdp2Controls.m4_pendingVdp2Regs->m108_CCRNA = (vdp2Controls.m4_pendingVdp2Regs->m108_CCRNA & 0xFFE0) | (arg << 0);\n            break;"
   "        case m45_COEN:\n            vdp2Controls.m4_pendingVdp2Regs->m110_CLOFEN = (vdp2Controls.m4_pendingVdp2Regs->m110_CLOFEN & ~0x1) | (arg << 0);\n            break;"
   source_text "${source_text}")

  # Original setupNBG3 also accepts all seven high-byte WCTLB controls.
  # MENUBK setup uses 33/34/35/37/38; the reconstruction asserted on them.
  string(REPLACE "#include \"lagi/lagi_azel_upstream_prelude.h\""
   "#include \"lagi/lagi_azel_upstream_prelude.h\"\n#include \"lagi/lagi_vdp2_window.h\""
   source_text "${source_text}")
  set(nbg3_window_old [=[        case m40_CAOS:
            vdp2Controls.m4_pendingVdp2Regs->mE4_CRAOFA = (vdp2Controls.m4_pendingVdp2Regs->mE4_CRAOFA & 0x8FFF) | (arg << 12);
            break;
        default:]=])
  set(nbg3_window_new [=[        case m40_CAOS:
            vdp2Controls.m4_pendingVdp2Regs->mE4_CRAOFA = (vdp2Controls.m4_pendingVdp2Regs->mE4_CRAOFA & 0x8FFF) | (arg << 12);
            break;
        case 33: case 34: case 35: case 36: case 37: case 38: case 39:
            vdp2Controls.m4_pendingVdp2Regs->mD2_WCTLB = lagi::native_nbg3_window(
                vdp2Controls.m4_pendingVdp2Regs->mD2_WCTLB, command, arg);
            break;
        default:]=])
  string(FIND "${source_text}" "${nbg3_window_old}" nbg3_window_offset)
  if(nbg3_window_offset LESS 0)
   message(FATAL_ERROR "Native NBG3 window adapter missing")
  endif()
  string(REPLACE "${nbg3_window_old}" "${nbg3_window_new}" source_text "${source_text}")
  set(nbg2_window_old [=[        case m34_W0E:
            vdp2Controls.m4_pendingVdp2Regs->mD2_WCTLB = (vdp2Controls.m4_pendingVdp2Regs->mD2_WCTLB & 0xFFFD) | (arg << 1);
            break;
        case m37_W0A:
            vdp2Controls.m4_pendingVdp2Regs->mD2_WCTLB = (vdp2Controls.m4_pendingVdp2Regs->mD2_WCTLB & 0xFFFE) | (arg << 0);
            break;]=])
  set(nbg2_window_new [=[        case 33: case 34: case 35: case 36: case 37: case 38: case 39:
            vdp2Controls.m4_pendingVdp2Regs->mD2_WCTLB = lagi::native_nbg2_window(
                vdp2Controls.m4_pendingVdp2Regs->mD2_WCTLB, command, arg);
            break;]=])
  string(FIND "${source_text}" "${nbg2_window_old}" nbg2_window_offset)
  if(nbg2_window_offset LESS 0)
   message(FATAL_ERROR "Native NBG2 window adapter missing")
  endif()
  string(REPLACE "${nbg2_window_old}" "${nbg2_window_new}" source_text "${source_text}")

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
 elseif(relative_path STREQUAL "town/e006/twn_e006.cpp" OR
        relative_path STREQUAL "town/townDragon.cpp" OR
        relative_path STREQUAL "town/excaEntity.cpp")
  # Reused dragon models do not run init3DModelRawData again. Trace their
  # real task draw boundary independently of the initialization diagnostic.
  if(relative_path STREQUAL "town/e006/twn_e006.cpp")
   set(actor_label "E006Dragon")
   set(actor_draw "submitModelAndShadowModelToRendering(&gDragonState->m28_dragon3dModel, gDragonState->m14_modelIndex, gDragonState->m18_shadowModelIndex, &gDragonCutscenePosition, &gDragonCutsceneRotation, 0)")
  elseif(relative_path STREQUAL "town/townDragon.cpp")
   set(actor_label "TownDragon")
   set(actor_draw "submitModelAndShadowModelToRendering(&gDragonState->m28_dragon3dModel, gDragonState->m14_modelIndex, gDragonState->m18_shadowModelIndex, &pThis->m58_position, &pThis->m64_rotation, 0)")
  else()
   set(actor_label "ExcaNPC")
   set(actor_draw "pThis->m10_3dModel.m18_drawFunction(&pThis->m10_3dModel)")
  endif()
  string(FIND "${source_text}" "${actor_draw};" actor_draw_marker)
  if(actor_draw_marker LESS 0)
   message(FATAL_ERROR "Native actor draw trace anchor missing: ${relative_path}")
  endif()
  string(REPLACE "${actor_draw};"
   "LAGI_TRACE_ACTOR_DRAW(\"${actor_label}\", ${actor_draw});"
   source_text "${source_text}")
  # These town actors can draw before townMainLogic publishes this frame's
  # native view. Their stack root already contains the town camera. Strip
  # that root at submission, just as townEdge does, rather than treating
  # camera-space geometry as world-space and applying a second camera.
  if(relative_path STREQUAL "town/townDragon.cpp" OR
     relative_path STREQUAL "town/e006/twn_e006.cpp")
   string(REPLACE
    "LAGI_TRACE_ACTOR_DRAW(\"${actor_label}\", ${actor_draw});"
    "lagi::azel_bridge::begin_view_relative_submission_scope();\n        LAGI_TRACE_ACTOR_DRAW(\"${actor_label}\", ${actor_draw});\n        lagi::azel_bridge::end_view_relative_submission_scope();"
    source_text "${source_text}")
  elseif(relative_path STREQUAL "town/excaEntity.cpp")
   # Capture before NPC translation/rotation, not at the hierarchy call.
   string(REPLACE
    "static void Draw2(sGenericTownNPC* pThis)\n    {\n        pushCurrentMatrix();"
    "static void Draw2(sGenericTownNPC* pThis)\n    {\n        lagi::azel_bridge::begin_view_relative_submission_scope();\n        pushCurrentMatrix();"
    source_text "${source_text}")
   string(REPLACE
    "        popMatrix();\n    }\n\n    static void Delete(sGenericTownNPC* pThis)"
    "        popMatrix();\n        lagi::azel_bridge::end_view_relative_submission_scope();\n    }\n\n    static void Delete(sGenericTownNPC* pThis)"
    source_text "${source_text}")
  endif()
  if(NOT source_text MATCHES "begin_view_relative_submission_scope" OR
     NOT source_text MATCHES "end_view_relative_submission_scope")
   message(FATAL_ERROR "Native town actor view scope did not apply: ${relative_path}")
  endif()
  string(PREPEND source_text [=[#include "lagi/platform.h"
#include "lagi/lagi_render_bridge.h"
#define LAGI_TRACE_ACTOR_DRAW(label, expression) do { \
    static unsigned reports = 4; \
    if (reports) { \
        const unsigned before = lagi::azel_bridge::submission_count(); \
        lagi::azel_bridge::trace_actor_submission_matrix(label); \
        expression; \
        lagi::platform::logging::writef("[NativeActorDraw] kind=%s submissions=%u\n", \
            label, lagi::azel_bridge::submission_count() - before); \
        --reports; \
    } else { expression; } \
} while (false)
]=])
 elseif(relative_path STREQUAL "mainMenuDebugTasks.cpp")
  set(menu_font_delete_old "    // clearSaveText(graphicEngineStatus.m40AC.fontIndex) — unloads menu font")
  set(menu_font_delete_new "    lagiAzelUnloadFontByIndex(graphicEngineStatus.m40AC.fontIndex); // LAGI_MENU_FONT_DELETE: native 0601579A.")
  string(FIND "${source_text}" "${menu_font_delete_old}" menu_font_delete_position)
  if(menu_font_delete_position LESS 0)
   message(FATAL_ERROR "Native outer-menu font cleanup target changed")
  endif()
  string(REPLACE "${menu_font_delete_old}" "${menu_font_delete_new}" source_text "${source_text}")
  string(REPLACE
   "    PDS_unimplemented(\"Unimplemented unloadFnt\");"
   "    lagiAzelUnloadCurrentFont();"
   source_text "${source_text}")
  # Report unsupported native actor variants rather than silently losing
  # their models in SHIPPING_BUILD. Do not substitute a different draw path.
  string(PREPEND source_text "#include \"lagi/platform.h\"\n")
  string(REPLACE
   [=[void unimplementedDraw(s_3dModel* pDragonStateData1)
{
    assert(0);
}]=]
   [=[void unimplementedDraw(s_3dModel* model)
{
    static const s_3dModel* reported[16]{};
    static unsigned count = 0;
    for (unsigned i = 0; i < count; ++i)
        if (reported[i] == model) return;
    if (count == 16) return;
    reported[count++] = model;
    lagi::platform::logging::writef(
        "[NativeActorDrawMissing] model=%p flags=%08X hierarchy=%08X bones=%u hotpoints=%u\n",
        model, static_cast<unsigned>(model->m8),
        static_cast<unsigned>(model->mC_modelIndexOffset),
        static_cast<unsigned>(model->m3C_boneMatrices.size()),
        model->m40 != nullptr);
}]=]
   source_text "${source_text}")
  string(REPLACE
   "    p3dModel->m1C_addToDisplayListFunction = addObjectToDrawList;"
   [=[    p3dModel->m1C_addToDisplayListFunction = addObjectToDrawList;
    static unsigned actorInitReports = 0;
    if (actorInitReports < 32u) {
        ++actorInitReports;
        lagi::platform::logging::writef(
            "[NativeActorModelReady] model=%p hierarchy=%04X flags=%04X animation=%04X bones=%u matrix=%u color=%u hotpoints=%u\n",
            p3dModel, static_cast<unsigned>(modelIndexOffset),
            static_cast<unsigned>(p3dModel->m8),
            static_cast<unsigned>(p3dModel->mA_animationFlags),
            static_cast<unsigned>(p3dModel->m12_numBones),
            static_cast<unsigned>(p3dModel->m3C_boneMatrices.size()),
            p3dModel->m38_pColorAnim != nullptr,
            p3dModel->m40 != nullptr);
    }]=]
   source_text "${source_text}")
  # Matrix-driven models without hotpoints select draw function 3. Upstream
  # leaves that variant empty, although it needs the same hierarchy traversal.
  string(REPLACE
   [=[void modelDrawFunction3(s_3dModel* pModel)
{
    unimplementedDraw(pModel);
}]=]
   [=[static void lagiDrawMatrixHierarchy(sModelHierarchy* node,
    s_3dModel* model, unsigned& bone)
{
    while (node && bone < model->m3C_boneMatrices.size()) {
        pushCurrentMatrix();
        multiplyCurrentMatrix(&model->m3C_boneMatrices[bone]);
        if (node->m0_3dModel) {
            if (model->m8 & 1)
                addObjectToDrawList(node->m0_3dModel);
            else
                addBillBoardToDrawList(node->m0_3dModel);
        }
        ++bone;
        if (node->m4_subNode)
            lagiDrawMatrixHierarchy(node->m4_subNode, model, bone);
        popMatrix();
        node = node->m8_nextNode;
    }
}
void modelDrawFunction3(s_3dModel* pModel)
{
    unsigned bone = 0;
    lagiDrawMatrixHierarchy(pModel->m4_pModelFile->getModelHierarchy(
        pModel->mC_modelIndexOffset), pModel, bone);
}]=]
   source_text "${source_text}")
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

  # The Vita map child is linked at Azel\'s existing dispatch boundary.
  # Keep upstream\'s assertion as a weak fallback, never rewrite Azel logic.
  string(REPLACE
   "p_workArea createMapTask(p_workArea workArea)"
   "__attribute__((weak)) p_workArea createMapTask(p_workArea workArea)"
   source_text "${source_text}")

  # Expose the actual native unavailable-slot task without mirroring its
  # private C++ work-area layout in a platform adapter. The caller owns the
  # coordinate storage until this child is destroyed.
  string(FIND "${source_text}" "mainMenuTaskInitSub2TaskWorkArea* mainMenuTaskInitSub2(p_workArea typelessWorkArea, sMainMenuTaskInitData2* r5, u32 r6)" lagi_native_cover_factory)
  if(lagi_native_cover_factory LESS 0)
   message(FATAL_ERROR "Native menu unavailable-cover factory no longer matches")
  endif()
  string(APPEND source_text [=[

// LAGI_NATIVE_MENU_COVER_BRIDGE_BEGIN
p_workArea lagiAzelCreateNativeMenuCover(p_workArea parent,
    sMainMenuTaskInitData2* config,u32 slot,u32 style)
{
    if(!parent || !config || !config->m0 || !config->m4 || slot>=2 || style>=2)
        return nullptr;
    auto* child=mainMenuTaskInitSub2(parent,config,style);
    if(child) child->spriteIndex=slot;
    return child;
}
// LAGI_NATIVE_MENU_COVER_BRIDGE_END
]=])

  # Native MAIN 06019F18 / COMMON task definition20FA0C restores the saved
  # cadence byte. The pinned upstream Delete remains an assertion-only stub.
  string(REPLACE
   "void s_mainMenuWorkArea::Delete(s_mainMenuWorkArea*)\n{\n    PDS_unimplemented(\"mainMenuTaskDelete\");\n}"
   "void s_mainMenuWorkArea::Delete(s_mainMenuWorkArea* pWorkArea)\n{\n    // LAGI_STATUS_MENU_RESTORE_CADENCE\n    vblankData.m14_numVsyncPerFrame = static_cast<u8>(pWorkArea->m1);\n}"
   source_text "${source_text}")
  string(FIND "${source_text}" "LAGI_STATUS_MENU_RESTORE_CADENCE" lagi_menu_cadence_restore)
  if(lagi_menu_cadence_restore LESS 0)
   message(FATAL_ERROR "Native status-menu cadence restore adapter no longer matches")
  endif()

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

  # Do not expose the removed temporary System menu as native functionality.
  string(REPLACE
   "    pWorkArea->m3_menuButtonStates[4] = 1; // setup"
   "    pWorkArea->m3_menuButtonStates[4] = -1; // native System task not ported yet"
   source_text "${source_text}")
  # Inventory is backed; remaining constructors still need native ports.
  # Keep Azel's real inventory task, but do not advertise the two constructors
  # that are still assert-only stubs in the pinned upstream revision.
  string(REPLACE
   "    if (mainGameState.gameStats.m20_XP) // defeated monsters menu\n    {\n        pWorkArea->m3_menuButtonStates[2] = 1;\n    }\n    else\n    {\n        pWorkArea->m3_menuButtonStates[2] = -1;\n    }\n    pWorkArea->m3_menuButtonStates[3] = 1; // map"
   "    pWorkArea->m3_menuButtonStates[2] = -1; // enemy list is an upstream stub\n    pWorkArea->m3_menuButtonStates[3] = 1; // Vita map child supports native menu dispatch"
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

 elseif(relative_path STREQUAL "3dEngine.cpp")
  # Native 0601FDF8/0601FE06 retain the stack pointer; 0601FE14 pops it.
  set(point_push_old [=[    u16* r2 = projectionStackTop;
    *(--r2) = loc_601FA9E;
    projectionStack[0] = *r2;]=])
  set(point_push_new [=[    assert(projectionStackTop > projectionStack);
    *--projectionStackTop = loc_601FA9E; // LAGI_NATIVE_SELECTOR_PUSH]=])
  string(FIND "${source_text}" "${point_push_old}" point_push_offset)
  if(point_push_offset LESS 0)
   message(FATAL_ERROR "Native projection selector push adapter missing")
  endif()
  string(REPLACE "${point_push_old}" "${point_push_new}" source_text "${source_text}")
  set(point_pop_old [=[    // popProjectionStack + Saturn VDP slave sync (not applicable on PC)
    Unimplemented();]=])
  set(point_pop_new [=[    assert(projectionStackTop < projectionStack + 8);
    loc_601FA9E = *projectionStackTop++; // LAGI_NATIVE_SELECTOR_POP]=])
  string(FIND "${source_text}" "${point_pop_old}" point_pop_offset)
  if(point_pop_offset LESS 0)
   message(FATAL_ERROR "Native projection selector pop adapter missing")
  endif()
  string(REPLACE "${point_pop_old}" "${point_pop_new}" source_text "${source_text}")
  set(point_activate_old [=[    PDS_unimplemented("dragonFieldTaskDrawSub1Sub0");]=])
  set(point_activate_new [=[    assert(projectionStackTop > projectionStack);
    *--projectionStackTop = loc_601FA9E;
    // 0601FDD0: A000 | ((0601FCDC - 0601FAA2) >> 1).
    loc_601FA9E = 0xA11D; // LAGI_NATIVE_POINT_SELECTOR]=])
  string(FIND "${source_text}" "${point_activate_old}" point_activate_offset)
  if(point_activate_offset LESS 0)
   message(FATAL_ERROR "Native point selector activation adapter missing")
  endif()
  string(REPLACE "${point_activate_old}" "${point_activate_new}" source_text "${source_text}")

 elseif(relative_path STREQUAL "field/fieldDragon.cpp")
  set(actor_base_anchor "            dragonFieldTaskDrawSub1Sub0();")
  string(FIND "${source_text}" "${actor_base_anchor}" actor_base_offset)
  if(actor_base_offset LESS 0)
   message(FATAL_ERROR "Field actor additive base-light anchor no longer matches")
  endif()
  string(REPLACE "${actor_base_anchor}" [=[            dragonFieldTaskDrawSub1Sub0();
            // LAGI_FIELD_ACTOR_ADDITIVE_BASE: same ordinary light as cleanup.
            sVec3_FP baseDirection, baseLight;
            baseDirection[0] = -MTH_Mul_5_6(fixedPoint(0x10000), getCos(pTypedWorkArea->mC0_lightRotationAroundDragon.getInteger()), getSin(pTypedWorkArea->mC4.getInteger()));
            baseDirection[1] = MTH_Mul(fixedPoint(0x10000), getSin(pTypedWorkArea->mC0_lightRotationAroundDragon.getInteger()));
            baseDirection[2] = -MTH_Mul_5_6(fixedPoint(0x10000), getCos(pTypedWorkArea->mC0_lightRotationAroundDragon.getInteger()), getCos(pTypedWorkArea->mC4.getInteger()));
            transformVecByCurrentMatrix(baseDirection, baseLight);
            lagi::azel_bridge::capture_field_actor_base_light(baseLight[0].asS32(),baseLight[1].asS32(),baseLight[2].asS32(),pTypedWorkArea->mC8_normalLightColor.toU32());]=]
   source_text "${source_text}")
  string(REPLACE
   "#include \"lagi/lagi_azel_upstream_prelude.h\""
   "#include \"lagi/lagi_azel_upstream_prelude.h\"\n#include \"lagi/lagi_render_bridge.h\""
   source_text "${source_text}")
  # User-requested extension: retain this frame's accepted special point
  # parameters for field environment consumers, without widening the native
  # projection-selector lifetime or changing dragon/rider rendering.
  set(environment_point_anchor [=[        generateLightFalloffMap(pTypedWorkArea->mCB_falloffColor0.toU32(), pTypedWorkArea->mCE_falloffColor1.toU32(), pTypedWorkArea->mD1_falloffColor2.toU32());]=])
  string(FIND "${source_text}" "${environment_point_anchor}" environment_point_offset)
  if(environment_point_offset LESS 0)
   message(FATAL_ERROR "Field environment point capture setup no longer matches")
  endif()
  string(REPLACE "${environment_point_anchor}"
   "${environment_point_anchor}\n        if (pTypedWorkArea->m_EB_useSpecialColor)\n            lagi::azel_bridge::capture_field_environment_point_light(); // LAGI_FIELD_ENVIRONMENT_POINT_SOURCE"
   source_text "${source_text}")
  # Native 06074020 calls 06036034 (point transform with translation).
  # The reconstructed vector-only transform left the light in a different
  # space from submitted view/model origins, producing outside-range zeros.
  set(point_position_old "transformVecByCurrentMatrix(varC, lightLocation);\n\n            dragonFieldTaskDrawSub1Sub1")
  set(point_position_new "transformAndAddVecByCurrentMatrix(&varC, &lightLocation); // LAGI_NATIVE_POINT_POSITION\n\n            dragonFieldTaskDrawSub1Sub1")
  string(FIND "${source_text}" "${point_position_old}" point_position_anchor)
  if(point_position_anchor LESS 0)
   message(FATAL_ERROR "Native point position transform anchor changed")
  endif()
  string(REPLACE "${point_position_old}" "${point_position_new}" source_text "${source_text}")
  # Original FLD_A3 06074026 loads R7=00014000 before calling
  # point-light setup at 0601E200. The reconstruction omitted R7.
  set(lagi_point_setup_original "dragonFieldTaskDrawSub1Sub1(lightLocation[0], lightLocation[1], lightLocation[2]);")
  set(lagi_point_setup_restored "dragonFieldTaskDrawSub1Sub1(lightLocation[0], lightLocation[1], lightLocation[2], 0x14000u);")
  string(FIND "${source_text}" "${lagi_point_setup_original}" lagi_point_setup_offset)
  if(lagi_point_setup_offset EQUAL -1)
   message(FATAL_ERROR "Native collection point-light argument adapter did not apply")
  endif()
  string(REPLACE "${lagi_point_setup_original}" "${lagi_point_setup_restored}"
   source_text "${source_text}")
  # Original FLD_A3 060741CC..06074290 pops the selector and restores the
  # ordinary directional vector/color after the dragon draw, under EC/EB.
  set(point_restore_old [=[        PDS_unimplemented("dragonFieldTaskDrawSub3 for setup light during dragon rendering");]=])
  set(point_restore_new [=[        dragonFieldTaskDrawSub3Sub1(); // LAGI_NATIVE_FIELD_LIGHT_RESTORE
        sVec3_FP direction;
        direction[0] = -MTH_Mul_5_6(fixedPoint(0x10000), getCos(pTypedWorkArea->mC0_lightRotationAroundDragon.getInteger()), getSin(pTypedWorkArea->mC4.getInteger()));
        direction[1] = MTH_Mul(fixedPoint(0x10000), getSin(pTypedWorkArea->mC0_lightRotationAroundDragon.getInteger()));
        direction[2] = -MTH_Mul_5_6(fixedPoint(0x10000), getCos(pTypedWorkArea->mC0_lightRotationAroundDragon.getInteger()), getCos(pTypedWorkArea->mC4.getInteger()));
        sVec3_FP lightLocation;
        transformVecByCurrentMatrix(direction, lightLocation);
        setupLight(lightLocation[0], lightLocation[1], lightLocation[2], pTypedWorkArea->mC8_normalLightColor.toU32());]=])
  string(FIND "${source_text}" "${point_restore_old}" point_restore_offset)
  if(point_restore_offset LESS 0)
   message(FATAL_ERROR "Native field light restoration adapter missing")
  endif()
  string(REPLACE "${point_restore_old}" "${point_restore_new}" source_text "${source_text}")

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
  set(navigator_default_old [=[        //06079234
        assert(0);]=])
  set(navigator_default_new [=[        // LAGI_NATIVE_NAVIGATOR_DEFAULT: 06079234, 060792BA.
        r14->m4_currentChoice = r5;
        createMultiChoiceSub1();
        r14->m0_choiceTable[r5] = 9;]=])
  string(FIND "${source_text}" "${navigator_default_old}" navigator_default_match)
  if(navigator_default_match LESS 0)
   message(FATAL_ERROR "Native navigator default-choice anchor changed")
  endif()
  string(REPLACE "${navigator_default_old}" "${navigator_default_new}" source_text "${source_text}")
  # Native 06073172: table at 060525D8 -> slot 2 -> first byte != 0.
  set(navigator_dispatch_anchor "    case 0x060730bc: // 060730bc — destination multichoice dialog")
  string(FIND "${source_text}" "${navigator_dispatch_anchor}" navigator_dispatch_match)
  if(navigator_dispatch_match LESS 0)
   message(FATAL_ERROR "Native navigator selected-text dispatch anchor changed")
  endif()
  string(REPLACE "${navigator_dispatch_anchor}"
   "    case 0x06073172: // LAGI_NATIVE_NAVIGATOR_SELECTED_TEXT\n        return lagiGlobalStringTableEntryHasText(2) ? 1 : 0;\n    case 0x06073120: // LAGI_NATIVE_NAVIGATOR_OPEN\n        return lagiOpenFieldNavigatorDialog();\n    case 0x06072f3c: // LAGI_NATIVE_NAVIGATOR_FINISH\n        return lagiFinishFieldNavigatorDialog();\n${navigator_dispatch_anchor}"
   source_text "${source_text}")
  set(navigator_text_old "selText = *(const char**)((u8*)pRadar->m1C_encounterList + pRadar->m18_currentSelection * 0x14);")
  set(navigator_text_new "selText = lagiFieldNavigatorText(*(u32*)((u8*)pRadar->m1C_encounterList + pRadar->m18_currentSelection * 0x14));")
  set(navigator_back_old "pAlloc[0] = 0; // placeholder for string pointer")
  set(navigator_back_new "pAlloc[0] = lagiFieldNavigatorDefaultTextEA(); // 06069214..1E")
  foreach(anchor IN ITEMS navigator_text_old navigator_back_old)
   string(FIND "${source_text}" "${${anchor}}" navigator_text_match)
   if(navigator_text_match LESS 0)
    message(FATAL_ERROR "Native navigator address anchor changed: ${anchor}")
   endif()
  endforeach()
  string(REPLACE "${navigator_text_old}" "${navigator_text_new}" source_text "${source_text}")
  string(REPLACE "${navigator_back_old}" "${navigator_back_new}" source_text "${source_text}")
  string(PREPEND source_text "#include \"lagi/lagi_particle_color_adapter.h\"\n")
  # Native FLD_A3: 0607AC3C indexes the trail table by frame*4 and
  # 0607AD44 selects an eight-byte orb block at 06094F64 by color type.
  # Restore omitted arguments, not renderer-side item color guesses.
  string(REPLACE
   "drawProjectedParticleWithGouraud(&pThis->m8_animQuad, &pThis->m10_position);"
   "lagiDrawParticleWithColorTable(&pThis->m8_animQuad, &pThis->m10_position, pThis->m1C_gouraudData, 4u);"
   source_text "${source_text}")
  string(REPLACE
   "drawProjectedParticleWithGouraud(&pThis->m58, &drawPos);"
   "lagiDrawParticleWithColorTable(&pThis->m58, &drawPos, gFLD_A3->getSaturnPtr(0x06094F64) + static_cast<unsigned>(pThis->m27) * 8u, 0u);"
   source_text "${source_text}")
  string(REGEX MATCHALL "lagiDrawParticleWithColorTable" lagi_collection_color_calls "${source_text}")
  list(LENGTH lagi_collection_color_calls lagi_collection_color_count)
  if(NOT lagi_collection_color_count EQUAL 2)
   message(FATAL_ERROR "Native collection color argument adapters did not apply")
  endif()
  string(PREPEND source_text "#include \"lagi/lagi_render_bridge.h\"\n")
  string(REPLACE
   "    r5->m14_index = index;"
   "    r5->m14_index = index;\n    if (r5->m0_memoryLayout.m0_mainMemoryBundle && r4->m30->m0_environmentGrid.size()) {\n        const auto bundleSlot = static_cast<std::int8_t>(std::min(2, static_cast<int>(r4->m30->mC)));\n        for (const auto& object : r4->m30->m0_environmentGrid[index]) {\n            for (unsigned lod = 0; lod < 4; ++lod) {\n                const auto offset = object.m0_modelsOffets[lod];\n                if (offset) lagi::azel_bridge::register_native_model_resource(bundleSlot, offset, r5->m0_memoryLayout.m0_mainMemoryBundle->get3DModel(offset));\n            }\n        }\n    }"
   source_text "${source_text}")
  if(NOT source_text MATCHES "register_native_model_resource")
   message(FATAL_ERROR "Field resource registration hook did not apply")
  endif()
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

  string(REPLACE
   "// LAGI_FLD_A3_STATIC_IDENTITY"
   "lagiState.fieldEnvironment = true; // LAGI_FIELD_ENVIRONMENT_POINT_TARGET\n                    // LAGI_FLD_A3_STATIC_IDENTITY"
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

  # Native ray calls terminate in Lagi's shared VDP1 effect service.

 elseif(relative_path STREQUAL "field.cpp")
  string(REPLACE
   "        pNewTask->m10 = 20;"
   "        pNewTask->m10 = 20;\n        pNewTask->m14_status = 0; // LAGI_FIELD_EXIT_ZERO_STATE: original cleared task contract"
   source_text "${source_text}")
  if(NOT source_text MATCHES "LAGI_FIELD_EXIT_ZERO_STATE")
   message(FATAL_ERROR "Native field-exit initialization anchor changed")
  endif()
  string(PREPEND source_text "#include \"lagi/lagi_render_bridge.h\"\n")
  string(REPLACE
   "}\n\nvoid getMemoryArea(s_memoryAreaOutput* pOutput, u32 areaIndex)"
   "    if (pFileData->MCB || pFileData->CGB) lagi::azel_bridge::notify_native_bundle_loaded(static_cast<std::int8_t>(slot));\n}\n\nvoid getMemoryArea(s_memoryAreaOutput* pOutput, u32 areaIndex)"
   source_text "${source_text}")
  if(NOT source_text MATCHES "notify_native_bundle_loaded")
   message(FATAL_ERROR "Native bundle generation hook did not apply")
  endif()
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
 elseif(relative_path STREQUAL "kernel/fade.cpp")
  # Invert unpackColor() when a running fade becomes a new request's source.
  # The original >>4 halves negative offsets (-128 becomes -64), making a
  # black scene pop halfway visible before Azel's next fade has advanced.
  string(REPLACE
   [=[    u32 color = ((inColor[0].toInteger() >> 4) + 0x10) << 10;
    color |= ((inColor[1].toInteger() >> 4) + 0x10) << 5;
    color |= ((inColor[2].toInteger() >> 4) + 0x10);
    color |= 0x8000;
    return color;]=]
   [=[    // LAGI_FADE_PACK_INVERSE
    const auto pack = [](s32 value) -> u32 {
        const s32 component = value < 0 ? value / 8 : value / 9;
        return static_cast<u32>(std::clamp(component + 0x10, 0, 0x1f));
    };
    return 0x8000u | pack(inColor[0].toInteger()) |
        (pack(inColor[1].toInteger()) << 5) |
        (pack(inColor[2].toInteger()) << 10);]=]
   source_text "${source_text}")
  string(REPLACE
   "#include \"fade.h\""
   "#include \"fade.h\"\n#include <algorithm>"
   source_text "${source_text}")
  if(NOT source_text MATCHES "LAGI_FADE_PACK_INVERSE")
   message(FATAL_ERROR "Native fade source-color correction did not apply")
  endif()
 elseif(relative_path STREQUAL "kernel/fileBundle.cpp")
  # Relocate addresses, never immediate colors. The reconstructed helper adds
  # the allocation base to CMDCOLR even for untextured RGB555 polygons.
  string(REPLACE
   "#include \"lagi/lagi_azel_upstream_prelude.h\""
   [=[#include "lagi/lagi_azel_upstream_prelude.h"
#include "processModel.h"
static void lagiPatchModelAddresses(sProcessed3dModel* model, u32 offset)
{
    for (auto& quad : model->mC_Quads) {
        const unsigned type = quad.mA_CMDCTRL & 15u;
        if (type <= 2u) {
            quad.m10_CMDSRCA += offset;
            if (((quad.mC_CMDPMOD >> 3) & 7u) == 1u)
                quad.mE_CMDCOLR += offset;
        }
    }
}
]=]
   source_text "${source_text}")
  string(REPLACE "pCachedData->patchFilePointers(mVdp1Offset);"
   "lagiPatchModelAddresses(pCachedData, mVdp1Offset);"
   source_text "${source_text}")
  string(REPLACE "pNewNode->m0_3dModel->patchFilePointers(mVdp1Offset);"
   "lagiPatchModelAddresses(pNewNode->m0_3dModel, mVdp1Offset);"
   source_text "${source_text}")
 elseif(relative_path STREQUAL "town/townCutscene.cpp")
  # Generic Vita bring-up for Azel's native in-engine town cutscene subsystem.
  # Azel retains ownership of script ordering, fades, camera commands, entity
  # animation, subtitles and completion. Lagi supplies only disc transport,
  # task allocator semantics, a stable 30 Hz timeline and lifecycle cleanup.
  string(REPLACE
   "#include \"lagi/lagi_azel_upstream_prelude.h\""
   "#include \"lagi/lagi_azel_upstream_prelude.h\"\n#include \"lagi/disc_image.h\"\n#include \"lagi/platform.h\"\n#include \"lagi/lagi_render_bridge.h\"\n#include <algorithm>\n#include <cstdint>\n#include <cstring>"
   source_text "${source_text}")

  # Streamed cameras replace the ordinary town camera during Draw. Capture
  # that authored view at its own boundary before subsequent model submissions.
  string(REPLACE
   "    updateEngineCamera(&cameraProperties2, local_10, local_18);"
   "    updateEngineCamera(&cameraProperties2, local_10, local_18);\n    lagi::azel_bridge::capture_native_scene_view_matrix();"
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
            "[AzelCutscene] missing stream file: %s\n", name.c_str());
        delete handle;
        return temp;
    }

    temp.fHandle = reinterpret_cast<FILE*>(handle);
    lagi::platform::logging::writef(
        "[AzelCutscene] opened %s (%u bytes) from mounted disc\n",
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
        lagi::platform::logging::writef("[AzelCutscene] stream allocation failed\n");
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
        "[AzelCutscene] request %s arg=%08X\n",
        streamFileName.c_str(), static_cast<unsigned>(arg));

    pThis->m0 = openFileForStreaming(&streamingParams, streamFileName);
    if (!pThis->m0)
    {
        lagi::platform::logging::writef(
            "[AzelCutscene] open failed: %s\n", streamFileName.c_str());
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

    if (iParm1->m28.m0 != 5)
        lagi::platform::logging::writef(
            "[AzelCutscene] complete state=%d frame=%u total=%u consumed=%u read=%u\n",
            iParm1->m28.m0, iParm1->m28.m84_frameIndex,
            iParm1->m194 ? iParm1->m194->m10_totalFrames : 0u,
            iParm1->m28.m28, iParm1->m28.m24_currentPositionInFile);
    iParm1->m198.m8_readIsPending = 0;
    iParm1->m28.m0 = 5;
}]=]
   source_text "${source_text}")

  # Preserve all four bytes of a wrapped command header. The reconstructed
  # U16 write corrupts a U32 packet length/opcode at command-ring rollover.
  string(REPLACE
   "WRITE_BE_U16(puVar3, READ_BE_U32(puVar4));"
   "WRITE_BE_U32(puVar3, READ_BE_U32(puVar4)); // LAGI_STREAM_HEADER_U32"
   source_text "${source_text}")
  if(NOT source_text MATCHES "LAGI_STREAM_HEADER_U32")
   message(FATAL_ERROR "Cutscene command-ring header correction did not apply")
  endif()

  # The host s_fileBundle is a C++ wrapper, separate from its DRAM payload.
  # Saturn teardown must release the raw allocation and delete the wrapper.
  string(REPLACE
   "            dramFree((u8*)pThis->m0);"
   "            dramFree(pThis->m0->getRawBuffer());\n            delete pThis->m0;\n            pThis->m0 = nullptr;"
   source_text "${source_text}")
  string(REPLACE
   "            vdp1Free((u8*)pThis->m4);"
   "            vdp1Free((u8*)pThis->m4);\n            pThis->m4 = nullptr;"
   source_text "${source_text}")

  # Correct the model-bundle base used by streamed entities as well as their
  # hierarchy. Otherwise get3DModel() patches texture addresses from zero.
  string(REPLACE
   "        //patchFilePointers(iVar4->getRawBuffer(), uVar3);"
   "        piVar2->m0->setPatchFilePointers(uVar3);"
   source_text "${source_text}")
  string(REPLACE
   "    pHierarchy->patchFilePointers(piVar2->m4->m4_vdp1Memory);"
   "    // The bundle patches each hierarchy model once at its native VRAM base."
   source_text "${source_text}")
  # Sector reads contain zero padding beyond EOF. Never expose that padding
  # to the packet parser: a zero length otherwise repeats forever in shipping
  # builds, where the unknown-opcode assert cannot stop the loop.
  string(REPLACE
   "        u32 uVar2 = (psParm1->m28).m24_currentPositionInFile - (psParm1->m28).m28;"
   [=[        const u32 readableEnd = std::min(
            psParm1->m28.m24_currentPositionInFile,
            static_cast<u32>(psParm1->m198.m18_fileSize));
        u32 uVar2 = readableEnd > psParm1->m28.m28
            ? readableEnd - psParm1->m28.m28 : 0u; // LAGI_STREAM_EOF_BOUND]=]
   source_text "${source_text}")
  string(REPLACE
   "        if ((int)uVar2 < READ_BE_U32((psParm1->m28).m34_pBufferRead)) {"
   [=[        cutsceneFillCommandBuffer(psParm1, psParm1->m28.m34_pBufferRead);
        const u32 packetSize = READ_BE_U32(psParm1->m28.m34_pBufferRead);
        if (packetSize < 0x10u || (packetSize & 3u) != 0u ||
            packetSize > psParm1->m4_bufferSize - psParm1->m28.m44_headerSize)
        {
            lagi::platform::logging::writef(
                "[AzelCutscene] invalid packet size=%u consumed=%u read=%u frame=%u\n",
                packetSize, psParm1->m28.m28, readableEnd,
                psParm1->m28.m84_frameIndex);
            psParm1->m28.m0 = -1;
            return;
        }
        if (uVar2 < packetSize) {]=]
   source_text "${source_text}")
  if(NOT source_text MATCHES "LAGI_STREAM_EOF_BOUND")
   message(FATAL_ERROR "Cutscene EOF bound did not apply")
  endif()
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

    if ((psParm1->m28.m84_frameIndex % 120u) < (frameRate + 29u) / 30u)
        lagi::platform::logging::writef(
            "[AzelCutscene] timeline state=%d frame=%u total=%u commandFrame=%u consumed=%u read=%u\n",
            psParm1->m28.m0, psParm1->m28.m84_frameIndex,
            psParm1->m194 ? psParm1->m194->m10_totalFrames : 0u,
            psParm1->m28.m88, psParm1->m28.m28,
            psParm1->m28.m24_currentPositionInFile);
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
 elseif(relative_path STREQUAL "town/exca/twn_exca.cpp")
  # VDP2 pattern names are Saturn big-endian bytes, unlike host-generated
  # rotation/coefficient work areas. Preserve the authored 0x5000 entries
  # without native little-endian u32 stores changing them into 0x0050.
  string(REPLACE
   [=[        u32* pVram = (u32*)getVdp2Vram(0x60800);
        for (int i = 0; i < 0x200; i++)
        {
            pVram[0] = 0x50005000;
            pVram[1] = 0x50005000;
            pVram[2] = 0x50005000;
            pVram[3] = 0x50005000;
            pVram += 4;
        }]=]
   [=[        for (u32 patternOffset = 0; patternOffset < 0x2000; patternOffset += 2)
        {
            setVdp2VramU16(0x60800 + patternOffset, 0x5000);
        }]=]
   source_text "${source_text}")
  if(NOT source_text MATCHES "setVdp2VramU16\\(0x60800 \\+ patternOffset, 0x5000\\)")
   message(FATAL_ERROR "Excavation VDP2 pattern endian adapter did not apply")
  endif()
 elseif(relative_path STREQUAL "town/townMainLogic.cpp")
  string(REPLACE
   "#include \"lagi/lagi_azel_upstream_prelude.h\""
   "#include \"lagi/lagi_azel_upstream_prelude.h\"\n#include \"lagi/lagi_render_bridge.h\""
   source_text "${source_text}")
  string(REPLACE
   "    generateCameraMatrix(&cameraProperties2, pThis->m38_interpolatedCameraPosition, var18, pThis->m50_upVector);"
   "    generateCameraMatrix(&cameraProperties2, pThis->m38_interpolatedCameraPosition, var18, pThis->m50_upVector);\n    lagi::azel_bridge::capture_native_scene_view_matrix();"
   source_text "${source_text}")
  if(NOT source_text MATCHES "capture_native_scene_view_matrix")
   message(FATAL_ERROR "Native town view capture did not apply")
  endif()
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
    [=[    static void DrawLoad(sSaveTask* pThis)
    {]=]
    [=[    static void DrawLoad(sSaveTask* pThis)
    {
        // LAGI_LOAD_TRANSITION_TRACE: bounded, read-only state-change evidence.
        static s32 traceState = -1;
        static s32 traceOperation = -1;
        static unsigned traceBudget = 24;
        if (traceBudget && (traceState != pThis->m4_status || traceOperation != pThis->m38_operationResult))
        {
            const auto& regs = vdp2Controls.m20_registers[0];
            lagi::platform::logging::writef(
                "[LoadTransition] state=%d operation=%d device=%d slot=%d version=%08X CLOFEN=%04X CLOFSL=%04X A=(%d,%d,%d) B=(%d,%d,%d) fadeCounter=%d stopped=%d\n",
                int(pThis->m4_status), int(pThis->m38_operationResult), int(pThis->m14_selectedDevice),
                int(pThis->m20_selectedFileInDevice), unsigned(gSaveGameStatus.m4_version),
                unsigned(regs.m110_CLOFEN), unsigned(regs.m112_CLOFSL),
                int(regs.m114_COAR), int(regs.m116_COAG), int(regs.m118_COAB),
                int(regs.m11A_COBR), int(regs.m11C_COBG), int(regs.m11E_COBB),
                int(g_fadeControls.m0_fade0.m1E_counter), int(g_fadeControls.m0_fade0.m20_stopped));
            lagi::platform::logging::writef(
                "[LoadTransitionMap] BGON=%04X CHCTLA=%04X CHCTLB=%04X MPOFN=%04X MPABN0=%04X MPABN1=%04X MPABN3=%04X\n",
                unsigned(regs.m20_BGON), unsigned(regs.m28_CHCTLA), unsigned(regs.m2A_CHCTLB),
                unsigned(regs.m3C_MPOFN), unsigned(regs.m40_MPABN0), unsigned(regs.m44_MPABN1), unsigned(regs.m4C_MPABN3));
            traceState = pThis->m4_status;
            traceOperation = pThis->m38_operationResult;
            --traceBudget;
        }]=]
    source_text "${source_text}")
   string(REPLACE "#include \"loadSavegameScreen.h\""
    "#include \"loadSavegameScreen.h\"\n#include \"lagi/platform.h\""
    source_text "${source_text}")
   if(NOT source_text MATCHES "LAGI_LOAD_TRANSITION_TRACE")
    message(FATAL_ERROR "Native load-transition trace anchor changed")
   endif()
   # Native 06026DF8 mini-data +4 is saved play frames, not the device ID.
   string(REPLACE "    s32 m4_device;\n    std::string m8_playerName;"
    "    s32 m4_playFrames; // LAGI_SAVE_PREVIEW_TIME\n    std::string m8_playerName;"
    source_text "${source_text}")
   string(REPLACE "            outputData->m4_device = device;"
    "            outputData->m4_playFrames = loadedGameState->mC_mainGameState.gameStats.m3C_frameCounter;"
    source_text "${source_text}")
   # Native 060191C4, including the US executable's exact format strings.
   # Native 060192C2: pre-tested count loop, COMMON F734, helper 06015BFC.
   string(REPLACE
    [=[    static void saveScreenDisplaySlotData2(s16*, s32)
    {
        Unimplemented();
    }]=]
    [=[    static void saveScreenDisplaySlotData2(s16* position, s32 dragonLevel)
    {
        // LAGI_SAVE_PREVIEW_RANK: valid native range 0..7; reject corrupt data.
        if (dragonLevel < 0 || dragonLevel > 7) return;
        auto data = gCommonFile->getSaturnPtr(0x20F734);
        for (s32 i = 0; i < dragonLevel; ++i, data = data + 8)
        {
            const s_menuSprite sprite = {
                readSaturnS16(data), readSaturnS16(data + 2),
                readSaturnS16(data + 4), readSaturnS16(data + 6)};
            drawMenuSprite(&sprite, position[0], position[1], 0x760);
        }
    }]=]
    source_text "${source_text}")
   if(NOT source_text MATCHES "LAGI_SAVE_PREVIEW_RANK")
    message(FATAL_ERROR "Native save rank anchor changed")
   endif()
   # Native slot strings follow the rank routine's generated-source patch.
   string(REPLACE
    [=[            drawObjectName(pSaveData->m4_miniData.m8_playerName.c_str());
            break;]=]
    [=[            drawObjectName(pSaveData->m4_miniData.m8_playerName.c_str());
            // LAGI_SAVE_PREVIEW_FIELDS: native player/dragon/time/rider level.
            vdp2StringContext.m4_cursorX = vdp2StringContext.mC_X + 2;
            vdp2StringContext.m8_cursorY = vdp2StringContext.m10_Y + 3;
            drawObjectName(pSaveData->m4_miniData.m19_dragonName.c_str());
            if (pThis->m20_stringRenderingX < 0x18)
            {
                const s32 minutes = std::min<s32>(intDivide(0xE10, pSaveData->m4_miniData.m4_playFrames), 0x176F);
                const s32 hours = intDivide(60, minutes);
                vdp2PrintStatus.m10_palette = 0xC000;
                vdp2DebugPrintSetPosition(pThis->m20_stringRenderingX + 0x14, pThis->m22_stringRenderingY + 1);
                vdp2PrintfLargeFont("%2d:%02d", hours, minutes - hours * 60);
                vdp2DebugPrintSetPosition(pThis->m20_stringRenderingX + 0x14, pThis->m22_stringRenderingY + 3);
                vdp2PrintfLargeFont("LV %2d", static_cast<int>(pSaveData->m4_miniData.m0_level) + 1);
            }
            break;]=]
    source_text "${source_text}")
   if(NOT source_text MATCHES "LAGI_SAVE_PREVIEW_FIELDS" OR NOT source_text MATCHES "LAGI_SAVE_PREVIEW_TIME")
    message(FATAL_ERROR "Native save-preview anchors changed")
   endif()
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
    "                auto* unavailable = createSubTask<sLagiUnavailableBackupDevice>(pThis);\n                unavailable->device = i;"
    source_text "${source_text}")
   string(REPLACE
    "        //clearSaveText(pThis->m10); // TODO: unload SAVE.FNT"
    "        lagiAzelUnloadFontByIndex(pThis->m10); // Native 06019BD4 calls 060250E6."
    source_text "${source_text}")
   string(REPLACE
    "            DisplayMenuMsg(stringArray[pThis->m14_selectedDevice]);\n        }\n        if (loadDrawSub1Sub0"
    "            DisplayMenuMsg(stringArray[pThis->m14_selectedDevice]);\n            return; // LAGI_SAVE_CANCEL_CURSOR: do not reactivate the hidden file cursor.\n        }\n        if (loadDrawSub1Sub0"
    source_text "${source_text}")
   if(NOT source_text MATCHES "LAGI_SAVE_CANCEL_CURSOR")
    message(FATAL_ERROR "Save file-selector cancellation adapter did not apply")
   endif()
   string(REPLACE
    "            pThis->m14 = 0;\n            pThis->m10_status++;\n        case 3:"
    "            pThis->m14 = 9; // LAGI_SAVE_SLOT_EXIT: native 060193AE\n            pThis->m10_status++;\n        case 3:"
    source_text "${source_text}")
   if(NOT source_text MATCHES "LAGI_SAVE_SLOT_EXIT")
    message(FATAL_ERROR "Save slot exit countdown adapter did not apply")
   endif()
   string(REPLACE
    "u16 saveMenuData_[]{"
    [=[// LAGI_UNAVAILABLE_BACKUP_DEVICE: native 060190BE/060190C8 task.
// COMMON.DAT 0020F724 supplies the covering sprite; 0020F770 is
// init data pointing to the device coordinates at 0020F764.
struct sLagiUnavailableBackupDevice : s_workAreaTemplate<sLagiUnavailableBackupDevice>
{
    static void Draw(sLagiUnavailableBackupDevice* pThis)
    {
        const auto spriteData = gCommonFile->getSaturnPtr(0x20F724);
        const s_menuSprite sprite = {
            readSaturnS16(spriteData), readSaturnS16(spriteData + 2),
            readSaturnS16(spriteData + 4), readSaturnS16(spriteData + 6)};
        const auto position = gCommonFile->getSaturnPtr(0x20F764) + pThis->device * 4;
        const auto& layer = graphicEngineStatus.m40BC_layersConfig[0];
        drawMenuSprite2(&sprite, readSaturnS16(position) - layer.scrollX,
            readSaturnS16(position + 2) - layer.scrollY, 0x500);
    }
    static TypedTaskDefinition* getTypedTaskDefinition()
    {
        static TypedTaskDefinition definition = {nullptr, nullptr, &Draw, nullptr};
        return &definition;
    }
    s32 device = 0;
};

u16 saveMenuData_[]{]=]
    source_text "${source_text}")
   string(REPLACE
    "                Unimplemented(); // TODO: setSoundOutputMode not yet implemented"
    "                // Audio output-mode restoration is deferred until that Saturn option is exposed."
    source_text "${source_text}")
   string(REPLACE
    "        Unimplemented();\n    }\n\n    // 06019bd4"
    "    }\n\n    // 06019bd4"
    source_text "${source_text}")
   string(REPLACE
    "            drawObjectName(locationTable[pSaveSlotInfo->m4_miniData.m2_location].c_str());"
    "            // LAGI_SAVE_LOCATION_BOUND\n            {\n                const auto location = static_cast<unsigned int>(pSaveSlotInfo->m4_miniData.m2_location);\n                drawObjectName(location < locationTable.size() ? locationTable[location].c_str() : \"Unknown location\");\n            }"
    source_text "${source_text}")
   string(REGEX MATCHALL "LAGI_SAVE_LOCATION_BOUND"
    lagi_save_location_matches "${source_text}")
   list(LENGTH lagi_save_location_matches lagi_save_location_count)
   if(NOT lagi_save_location_count EQUAL 1)
    message(FATAL_ERROR "Save location bound patch did not apply exactly once")
   endif()
   # Upstream's overwrite dialog is a stub that immediately proceeds to the
   # write. Restore the pending confirmation state without replacing the
   # native save task or touching existing files during startup/diagnostics.
   string(REPLACE
    [=[        default:
            assert(0);
        }
    case 1:
        DisplayMenuMsg("Will erase old file. Execute?\n");
        Unimplemented(); pThis->m10_status = 3;
        //pThis->m10_status++;
        break;
    case 2:
        if (pThis->m14)
        {
            return;
        }
        if (pThis->m18 == 0)
        {
            *pThis->mC_pOperationResult = 1;
            pThis->getTask()->markFinished();
            return;
        }
        pThis->m10_status++;
        break;]=]
    [=[        default:
            assert(0);
        }
        return; // Do not fall through: an empty slot needs no overwrite prompt.
    case 1:
        // LAGI_SAVE_OVERWRITE_CONFIRM
        DisplayMenuMsg("Overwrite old file? Confirm: Yes / Cancel: No");
        pThis->m10_status = 2;
        return; // Never reuse the button edge that selected the slot.
    case 2:
    {
        const auto buttons = graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown;
        if (buttons & 1) // Cancel wins if both actions are pressed.
        {
            playSystemSoundEffect(1);
            *pThis->mC_pOperationResult = 1;
            pThis->getTask()->markFinished();
            return;
        }
        if (buttons & 6)
        {
            playSystemSoundEffect(0);
            pThis->m10_status = 3;
        }
        return;
    }]=]
    source_text "${source_text}")
   string(REPLACE
    [=[            case 1:
                displaySaveLocation(pThis, saveStrings);
                break;
            default:
                assert(0);]=]
    [=[            case 1:
                displaySaveLocation(pThis, saveStrings);
                break;
            case 2: // LAGI_SAVE_WRITE_FAILURE_RECOVERY
                // Keep the native failure message and slot selector usable.
                playSystemSoundEffect(5);
                break;
            default:
                assert(0);]=]
    source_text "${source_text}")
   string(REGEX MATCHALL "LAGI_SAVE_WRITE_FAILURE_RECOVERY"
    lagi_save_failure_matches "${source_text}")
   list(LENGTH lagi_save_failure_matches lagi_save_failure_count)
   if(NOT lagi_save_failure_count EQUAL 1)
    message(FATAL_ERROR "Save failure recovery patch did not apply exactly once")
   endif()
   string(REGEX MATCHALL "LAGI_SAVE_OVERWRITE_CONFIRM"
    lagi_save_confirm_matches "${source_text}")
   list(LENGTH lagi_save_confirm_matches lagi_save_confirm_count)
   if(NOT lagi_save_confirm_count EQUAL 1)
    message(FATAL_ERROR "Save overwrite confirmation patch did not apply exactly once")
   endif()
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
  elseif(relative_path STREQUAL "menu/inventoryMenu.cpp")
   string(REPLACE
    "void useInventoryItem(s_inventoryMenu* pThis)\n{\n    Unimplemented();\n}"
    [=[void inventoryMenuTaskDrawSub1(s_inventoryMenu* pThis);
void inventoryMenuTaskDrawSub2(s_inventoryMenu* pThis, s32 item);
void inventoryMenuTaskDrawSub0(s_inventoryMenu* pThis, s32 categoryId);
static s16 lagiInventoryBerserkBPCost(s16 baseCost);

static bool lagiInventoryConsumeHeal(eItems item)
{
    // Native 0600FAB8: authoritative nine-entry table at COMMON 00208A20.
    for (int i = 0; i < 9; ++i)
    {
        const auto entry = gCommonFile->getSaturnPtr(0x208A20) + i * 6;
        if (readSaturnS16(entry) != item) continue;
        const s16 hpHeal = readSaturnS16(entry + 2);
        const s16 bpHeal = readSaturnS16(entry + 4);
        auto& stats = mainGameState.gameStats;
        if (!mainGameState.consumables[item] ||
            ((!hpHeal || stats.m10_currentHP >= stats.mB8_maxHP) &&
             (!bpHeal || stats.m14_currentBP >= stats.mBA_maxBP)))
            return false;
        --mainGameState.consumables[item];
        const s32 hp = stats.m10_currentHP + hpHeal;
        const s32 bp = stats.m14_currentBP + bpHeal;
        stats.m10_currentHP = static_cast<s16>(hp > stats.mB8_maxHP ? stats.mB8_maxHP : hp);
        stats.m14_currentBP = static_cast<s16>(bp > stats.mBA_maxBP ? stats.mBA_maxBP : bp);
        return true;
    }
    return false;
}

static s16 lagiInventoryBerserkHeal(eItems item)
{
    // Native 06007432 table at 06042E3C; 060073F8 fixed-point scaling.
    s16 base = 0;
    switch (item)
    {
    case 0xAA: base = 400; break;
    case 0xAB: base = 900; break;
    case 0xAC: base = 20000; break;
    default: return 0;
    }
    const fixedPoint spr = FP_Div((s32)mainGameState.gameStats.mC2_dragonSpr, 200);
    const fixedPoint scale = fixedPoint(0xCCCC) + MTH_Mul(fixedPoint(0x6666), spr);
    const fixedPoint result = MTH_Mul(fixedPoint::fromInteger(base), scale);
    return static_cast<s16>((result.asS32() + 0x8000) >> 16);
}

void useInventoryItem(s_inventoryMenu* pThis)
{
    // LAGI_NATIVE_INVENTORY_WEAPON_USE: native 06018570 / 0601869E.
    const int index = pThis->m4_currentPageInItemList * 5 + pThis->m24_currentSelectedItemInCurrentItemPage;
    if (index < 0 || index >= pThis->m30_numItemsInCurrentCategory ||
        index >= static_cast<int>(pThis->m34_itemsInCurrentCategory.size()))
        return;
    const auto item = pThis->m34_itemsInCurrentCategory[index];
    if (!(getObjectListEntry(item)->m0_flags & pThis->m10))
    {
        playSystemSoundEffect(5);
        return;
    }
    if (pThis->m1C_selectedCategory == 3)
    {
        mainGameState.gameStats.mA_weaponType =
            mainGameState.gameStats.mA_weaponType == item ? static_cast<eItems>(0) : item;
        playSystemSoundEffect(7);
        inventoryMenuTaskDrawSub1(pThis);
        inventoryMenuTaskDrawSub2(pThis, item);
        return;
    }
    if (pThis->m1C_selectedCategory == 2)
    {
        // Native 06018634..06018694: no BP spent unless healing succeeds.
        auto& stats = mainGameState.gameStats;
        if (stats.m10_currentHP >= stats.mB8_maxHP)
        {
            playSystemSoundEffect(5);
            return;
        }
        const s16 heal = lagiInventoryBerserkHeal(item);
        const s16 cost = lagiInventoryBerserkBPCost(readSaturnS16(
            gCommonFile->getSaturnPtr(0x20C3F4) + 12 * item + 2));
        if (!heal || cost > stats.m14_currentBP)
        {
            playSystemSoundEffect(5);
            return;
        }
        stats.m14_currentBP -= cost;
        const s32 hp = stats.m10_currentHP + heal;
        stats.m10_currentHP = static_cast<s16>(hp > stats.mB8_maxHP ? stats.mB8_maxHP : hp);
        playSystemSoundEffect(stats.m1_dragonLevel == 8 ? 0x21 : 0x1A);
        return;
    }
    if (pThis->m1C_selectedCategory == 0)
    {
        if (!lagiInventoryConsumeHeal(item))
        {
            playSystemSoundEffect(5);
            return;
        }
        playSystemSoundEffect(mainGameState.gameStats.m1_dragonLevel == 8 ? 0x21 : 0x1A);
        inventoryMenuTaskDrawSub0(pThis, 0);
        if (pThis->m30_numItemsInCurrentCategory == 0)
        {
            pThis->m2C_hasCategoryAtLeastOneEntry[0] = 0;
            switchVdp2MenuTileToHightlighted(inventoryMenuSpriteTable[0], 2, 2, 0x630);
            displayInventoryCategoryDescription(pThis);
            return;
        }
        // Native 06018420 rebuilds around the consumed item's old position.
        // Bound that position when the removed item was the final row.
        const int selected = index < pThis->m30_numItemsInCurrentCategory
            ? index : pThis->m30_numItemsInCurrentCategory - 1;
        pThis->m4_currentPageInItemList = selected / 5;
        pThis->m24_currentSelectedItemInCurrentItemPage = selected % 5;
        pThis->m20_menuCursorCategory->m0_selectedEntry = -1;
        pThis->m28_menuCursorItem->m0 = selected % 5;
        inventoryMenuTaskDrawSub1(pThis);
        inventoryMenuTaskDrawSub2(pThis, pThis->m34_itemsInCurrentCategory[selected]);
        return;
    }
    // Key-item native handlers remain to port.
    Unimplemented();
}]=]
    source_text "${source_text}")
   string(REPLACE
    "void inventoryMenuTaskDrawSub2(s_inventoryMenu* pThis, s32 param_2)\n{\n    Unimplemented();\n}"
    [=[void inventoryMenuTaskDrawSub2(s_inventoryMenu* pThis, s32 param_2)
{
    // LAGI_NATIVE_INVENTORY_DETAILS: native 06017F7A..06018052.
    setupVDP2StringRendering(3, 0x38, 0x26, 4);
    clearVdp2TextArea();
    if (param_2 < 0) return;
    const auto item = static_cast<eItems>(param_2);
    vdp2StringContext.m0 = 0;
    auto printCommon = [](u32 address) {
        printVdp2String2(readSaturnString(gCommonFile->getSaturnPtr(address)).c_str());
    };
    if (pThis->m1C_selectedCategory == 2)
    {
        drawObjectName(item == 0
            ? readSaturnString(gCommonFile->getSaturnPtr(0x20D882)).c_str()
            : getObjectListEntry(item)->m8_description.c_str());
        vdp2StringContext.m4_cursorX = vdp2StringContext.mC_X;
        vdp2StringContext.m8_cursorY = vdp2StringContext.m10_Y + 2;
        // Original pointer table at 0604329C, indexed by Berserk page.
        static const u32 classes[6] = {0x20D896, 0x20D8AF, 0x20D8CE,
            0x20D8E9, 0x20D904, 0x20D925};
        printCommon(classes[pThis->m4_currentPageInItemList]);
        return;
    }
    drawObjectName(getObjectListEntry(item)->m8_description.c_str());
    if (pThis->m1C_selectedCategory != 3) return;
    vdp2StringContext.m4_cursorX = vdp2StringContext.mC_X;
    vdp2StringContext.m8_cursorY = vdp2StringContext.m10_Y + 2;
    drawObjectName(readSaturnString(gCommonFile->getSaturnPtr(0x20D86C)).c_str());
    s16 modifier = 0;
    for (const auto& weapon : weaponTable)
    {
        if (weapon.m0_weaponType < 0) break;
        if (weapon.m0_weaponType != item) continue;
        const u32 product = (static_cast<u32>(weapon.m4.asS32()) - 0x10000u) * 100u + 0x8000u;
        modifier = static_cast<s16>(product >> 16);
        break;
    }
    int width = 4;
    if (modifier > 0)
    {
        printCommon(modifier > 99 ? 0x20D87A : 0x20D87D);
        width = modifier > 99 ? 3 : 2;
    }
    printVdp2Number2(modifier, width);
    printCommon(0x20D877);
}]=]
    source_text "${source_text}")
   string(REPLACE
    "#include \"items.h\""
    "#include \"items.h\"\n#include \"battle/gunShotRootTask.h\""
    source_text "${source_text}")
   string(REPLACE
    "    case 2: // berserks\n        assert(0);\n        break;"
    [=[    case 2: // LAGI_NATIVE_BERSERK_ROWS: native 06017C62..06017C9E.
        while (*currentCategoryItemList >= 0)
        {
            const auto item = static_cast<eItems>(*currentCategoryItemList++);
            *psVar4++ = mainGameState.getItemCount(item) ? item : static_cast<eItems>(0);
        }
        break;]=]
    source_text "${source_text}")
   string(REPLACE
    "void inventoryPrintBerserk(s32, eItems, s32)\n{\n    Unimplemented();\n}"
    [=[// Native 06007378; same fixed-point implementation as battleCommandMenu.cpp.
// That battle translation unit is not part of the Vita source set.
static s16 lagiInventoryBerserkBPCost(s16 baseCost)
{
    const fixedPoint spr = FP_Div((s32)mainGameState.gameStats.mC2_dragonSpr, 200);
    const fixedPoint reduction = MTH_Mul(fixedPoint(0x6666), spr);
    const fixedPoint result = MTH_Mul(fixedPoint::fromInteger(baseCost), fixedPoint(0x13333) - reduction);
    return (s16)((result.asS32() + 0x8000) >> 16);
}

void inventoryPrintBerserk(s32 row, eItems item, s32 contextFlags)
{
    // LAGI_NATIVE_BERSERK_PRINT: native 060181A0..06018284.
    const auto data = gCommonFile->getSaturnPtr(0x20C3F4) + 12 * item;
    const s16 cost = lagiInventoryBerserkBPCost(readSaturnS16(data + 2));
    if (item == 0) return;
    vdp2StringContext.m0 = 9;
    if ((getObjectListEntry(item)->m0_flags & contextFlags) &&
        mainGameState.gameStats.m1_dragonLevel < 8)
        vdp2StringContext.m0 = 0;
    displayObjectIcon(0, 0xC, row + 0x24, getObjectIcon(item));
    drawObjectName(getObjectListEntry(item)->m4_name.c_str());
    vdp2StringContext.m4_cursorX = vdp2StringContext.mC_X + 0x16;
    vdp2StringContext.m8_cursorY = vdp2StringContext.m10_Y + row +
        (row < 0 ? vdp2StringContext.m18_Height : 0);
    printVdp2Number2(cost, 3);
    printVdp2String2(readSaturnString(gCommonFile->getSaturnPtr(0x20D93C)).c_str());
}]=]
    source_text "${source_text}")
   set(inventory_delete_old "void inventoryMenuTaskDelete(s_inventoryMenu* pThis)\n{\n    Unimplemented();\n}")
   set(inventory_delete_new "void inventoryMenuTaskDelete(s_inventoryMenu* pThis)\n{\n    lagiAzelUnloadFontByIndex(pThis->mC); // LAGI_INVENTORY_FONT_DELETE: native 06018B4C.\n}")
   string(FIND "${source_text}" "${inventory_delete_old}" inventory_delete_position)
   if(inventory_delete_position LESS 0)
    message(FATAL_ERROR "Native inventory font cleanup target changed")
   endif()
   string(REPLACE "${inventory_delete_old}" "${inventory_delete_new}" source_text "${source_text}")
   string(REPLACE
    "        if (i < pThis->m30_numItemsInCurrentCategory) {"
    "        // LAGI_INVENTORY_PAGE_BOUND\n        const int itemIndex = 5 * pThis->m4_currentPageInItemList + i;\n        if (itemIndex >= 0 && itemIndex < pThis->m30_numItemsInCurrentCategory &&\n            itemIndex < static_cast<int>(pThis->m34_itemsInCurrentCategory.size())) {"
    source_text "${source_text}")
   string(REGEX MATCHALL "LAGI_INVENTORY_PAGE_BOUND"
    lagi_inventory_page_matches "${source_text}")
   list(LENGTH lagi_inventory_page_matches lagi_inventory_page_count)
   if(NOT lagi_inventory_page_count EQUAL 1)
    message(FATAL_ERROR "Inventory page bound patch did not apply exactly once")
   endif()
  elseif(relative_path STREQUAL "kernel/menuCursor.cpp")
   string(REPLACE
    "    if (pWorkArea->m0_selectedEntry < 0)\n        return;"
    "    if (graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown & 0x37)\n        PDS_Log(\"[MenuCursor] entry=%d blink=%d pressed=%04X\\n\", pWorkArea->m0_selectedEntry, pWorkArea->mC, graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown);\n    if (pWorkArea->m0_selectedEntry < 0)\n        return;"
    source_text "${source_text}")
   string(REPLACE
    "    if (pThis->m0 < 0)\n        return;"
    "    if (graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown & 0x37)\n        PDS_Log(\"[MenuCursor2] entry=%d frame=%d countdown=%d vsync=%d pressed=%04X\\n\", pThis->m0, pThis->m8, pThis->mC, vblankData.m14_numVsyncPerFrame, graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown);\n    if (pThis->m0 < 0)\n        return;"
    source_text "${source_text}")
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
 ${LAGI_STOCHASTIC_F_OBJ}
 ${LAGI_STOCHASTIC_GOURAUD_F_OBJ}
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
 src/integration/lagi_azel_books_menu.cpp
 src/integration/lagi_azel_books_resource.cpp
 src/integration/lagi_azel_map_menu.cpp
 src/integration/lagi_azel_map_context.cpp
 src/integration/lagi_map_menu_vita.cpp
 src/integration/lagi_map_message_vita.cpp
 src/integration/lagi_map_selection_vita.cpp
 src/integration/lagi_map_confirmation_vita.cpp
 src/integration/lagi_map_resources_vita.cpp
 src/integration/lagi_map_view_vita.cpp
 src/integration/lagi_azel_field_radar.cpp
 src/integration/lagi_azel_field_navigator.cpp
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
target_compile_definitions(lagi PRIVATE LAGI_PLATFORM_VITA=1 LAGI_NEPTUNE_RENDERER=1 USE_NULL_RENDERER=1 SHIPPING_BUILD=1 LAGI_FULLRES=$<BOOL:${LAGI_FULLRES}> LAGI_STOCHASTIC_FILTER=$<BOOL:${LAGI_STOCHASTIC_FILTER}> LAGI_DITHER_LOOKUP=$<BOOL:${LAGI_DITHER_LOOKUP}>)
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
target_link_libraries(lagi m SceAudio_stub SceCtrl_stub SceDisplay_stub SceGxm_stub SceKernelThreadMgr_stub SceProcessmgr_stub SceSysmem_stub SceSysmodule_stub)

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
