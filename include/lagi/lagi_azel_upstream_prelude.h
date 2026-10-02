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
