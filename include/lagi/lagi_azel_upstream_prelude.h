#pragma once

// Build adapter for compiling upstream Azel runtime sources on Vita.
//
// IMPORTANT ARCHITECTURE RULE:
// - Gameplay/runtime behavior stays in extern/Azel unchanged.
// - This header only supplies the platform-neutral engine declarations that
//   upstream PDS.h normally exposes alongside desktop SDL/BGFX/SoLoud/ImGui.
// - Do not add replacement gameplay logic here.

#ifndef SHIPPING_BUILD
#define SHIPPING_BUILD 1
#endif

#include "lagi/lagi_source_compat.h"

#include <cstdio>
#include <algorithm>
#include <cmath>

#include "dummy.h"

#include "heap.h"
#include "task.h"
#include "rootTask.h"
#include "VDP1.h"
#include "VDP2.h"
#include "titleScreen.h"
#include "common.h"
#include "3dEngine.h"
#include "3dEngine_flush.h"
#include "mainMenuDebugTasks.h"
#include "field.h"
#include "LCS.h"
#include "kernel/receiveItemTask.h"
#include "menu_dragon.h"
#include "menu_dragonMorph.h"
#include "3dModels.h"
#include "trigo.h"

#ifndef isShipping
#define isShipping() true
#endif


extern u8 COMMON_DAT[0x98000];

struct sFileInfoSub
{
    sFileInfoSub* pNext;
    char m_fileName[32];
    FILE* fHandle;
    u32 m_fileSize;
    u16 m_18;
    u32 m1A_vdp1Data;
};

struct sFileInfo
{
    u8 m0;
    u8 m1;
    u8 m2;
    u8 m3;
    u8 m4;
    u8 displayMemoryLayout;
    u16 m8;

    struct GfsHn* mC_gfsHandle;
    void* m10_movieDecoder;
    u32 m14_frameCount;
    void* m18_decodeBuffer;
    u32 m1C_vramDest;
    u32 m20_videoWidth;
    u32 m24_videoHeight;
    u32 m28_countdown;

    sFileInfoSub* m2C_allocatedHead;
    sFileInfoSub* freeHead;
    sFileInfoSub linkedList[15];
};

extern sFileInfo fileInfoStruct;

extern bool debugEnabled;
extern int enableDebugTask;
extern std::array<u8, 3> pauseEngine;
extern u8 gCurrentVDP2ScrollLayer;
extern u32 azelCdNumber;

void WRITE_BE_U16(const void* ptr, u16 value);
void WRITE_BE_U32(const void* ptr, u32 value);
sVec3_FP READ_BE_Vec3(const void* ptr);
u32 READ_BE_U32(const void* ptr);
s32 READ_BE_S32(const void* ptr);
u16 READ_BE_U16(const void* ptr);
s16 READ_BE_S16(const void* ptr);
u8 READ_BE_U8(const void* ptr);
s8 READ_BE_S8(const void* ptr);

u32 getFileSize(const char* fileName);

void initVDP1Projection(fixedPoint r4, u32 mode);
void getVdp1ProjectionParams(s16* r4, s16* r5);
void getVdp1ScreenResolution(s16 (&screenResolution)[4]);

s32 setDividend(s32 r4, s32 r5, s32 divisor);
fixedPoint sqrt_F(fixedPoint r4);
s32 sqrt_I(s32 r4);
void initFileLayoutTable();

extern bool hasEncounterData;
extern u8 encounterTaskVar0;
extern u8 townBuffer[0xB0000];

extern p_workArea (*gFieldOverlayFunction)(p_workArea workArea, u32 arg);

fixedPoint distanceSquareBetween2Points(
    const sVec3_FP& r4_vertice0,
    const sVec3_FP& r5_vertice1);

void RendererSetFov(float fovInDegree);
s32 MTH_Product2d(s32 (&r4)[2], s32 (&r5)[2]);
fixedPoint MulVec2(const sVec2_FP& r4, const sVec2_FP& r5);
void adjustMatrixTranslation(fixedPoint r4);
s32 udivsi3(s32 r0, s32 r1);

#ifndef DEG_80
#define DEG_80 (0x038e38e2)
#endif
#ifndef DEG_50
#define DEG_50 (0x0238e38e)
#endif


// PDS.cpp contains the desktop top-level frame loop, but Lagi owns the Vita
// host loop. These declarations exist only so that unused desktop loop code
// compiles and can be removed by --gc-sections.
void azelSdl_StartFrame();
bool azelSdl_EndFrame();
