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

set(LAGI_TEXTURE_V_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_texture_v.gxp")
set(LAGI_TEXTURE_F_GXP "${CMAKE_CURRENT_BINARY_DIR}/lagi_texture_f.gxp")
set(LAGI_TEXTURE_V_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_texture_v_gxp.o")
set(LAGI_TEXTURE_F_OBJ "${CMAKE_CURRENT_BINARY_DIR}/lagi_texture_f_gxp.o")

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
 "${LAGI_TEXTURE_V_OBJ}" "${LAGI_TEXTURE_F_OBJ}"
 PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE
)

add_executable(lagi
 src/main.cpp
 src/platform/vita/vita_platform.cpp
 src/platform/vita/vita_input.cpp
 src/platform/vita/vita_filesystem.cpp
 src/platform/vita/vita_log.cpp
 src/platform/vita/iso9660.cpp
 src/platform/vita/null_renderer.cpp
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
 ${LAGI_TEXTURE_V_OBJ}
 ${LAGI_TEXTURE_F_OBJ}
 src/platform/vita/null_audio.cpp
 src/integration/azel_runtime_smoke.cpp
 src/integration/azel_render_bridge.cpp
 src/integration/azel_live_model_adapter.cpp
 src/integration/azel_direct_boot.cpp
 src/integration/azel_town_bootstrap.cpp
 src/integration/azel_town_runtime.cpp
 src/integration/azel_town_collision.cpp
 src/integration/azel_town_tasks.cpp
 src/integration/azel_ruin_room_debug.cpp
 src/integration/azel_edge_debug.cpp
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
vita_create_vpk(Lagi.vpk ${VITA_TITLEID} lagi.self
 VERSION ${VITA_VERSION}
 NAME ${VITA_APP_NAME}
 FILE
  "${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/icon0.png" sce_sys/icon0.png
  "${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/livearea/contents/bg0.png" sce_sys/livearea/contents/bg0.png
  "${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/livearea/contents/startup.png" sce_sys/livearea/contents/startup.png
  "${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/livearea/contents/template.xml" sce_sys/livearea/contents/template.xml
)
