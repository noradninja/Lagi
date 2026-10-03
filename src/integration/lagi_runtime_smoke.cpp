#include <cstdio>
#include <psp2/kernel/processmgr.h>
#include "lagi/lagi_compat.h"
#include "heap.h"
#include "task.h"
#include "VDP1.h"
#include "VDP2.h"
#include "common.h"
#include "lagi/platform.h"
#include "lagi/debug_mesh.h"
#include "lagi/lagi_render_bridge.h"
#include "lagi/lagi_direct_boot.h"
#include "lagi/lagi_town_bootstrap.h"
#include "lagi/lagi_town_runtime.h"
#include "lagi/lagi_town_tasks.h"
#include "lagi/disc_image.h"
#include "commonOverlay.h"
#include "audio/soundDataTable.h"
#include "lagi/dragon_common.h"
#include <utility>
#include <vector>

extern int numActiveTask;
void initSMPC();
void initVDP1();
void iniitInitialTaskStatsAndDebugSub();
void writeInputConfig(s32 type, const std::array<s32, 8>& config, s32 inverseY);
void updateInputs();

namespace lagi::azel {

static bool saturn_memory_smoke_test()
{
    constexpr u32 base = 0x06054000;
    u8 data[40] = {};

    data[0] = 0x12; data[1] = 0x34;
    data[4] = 0x00; data[5] = 0x01; data[6] = 0x00; data[7] = 0x00;
    data[8] = 'A'; data[9] = 'Z'; data[10] = 'E'; data[11] = 'L'; data[12] = 0;

    const u32 target = base + 24;
    data[16] = static_cast<u8>(target >> 24);
    data[17] = static_cast<u8>(target >> 16);
    data[18] = static_cast<u8>(target >> 8);
    data[19] = static_cast<u8>(target);

    const s32 vec[3] = { 0x00010000, -0x00020000, 0x00008000 };
    for (int i = 0; i < 3; ++i) {
        const u32 v = static_cast<u32>(vec[i]);
        data[24 + i * 4 + 0] = static_cast<u8>(v >> 24);
        data[24 + i * 4 + 1] = static_cast<u8>(v >> 16);
        data[24 + i * 4 + 2] = static_cast<u8>(v >> 8);
        data[24 + i * 4 + 3] = static_cast<u8>(v);
    }

    sSaturnMemoryFile file;
    file.m_name = "Lagi synthetic Saturn block";
    file.m_data = data;
    file.m_dataSize = sizeof(data);
    file.m_base = base;

    const sSaturnPtr root = file.getSaturnPtr(base);
    if (readSaturnU16(root) != 0x1234) return false;
    if (readSaturnFP(root + 4).asS32() != 0x00010000) return false;
    if (readSaturnString(root + 8) != "AZEL") return false;

    const sSaturnPtr ea = readSaturnEA(root + 16);
    const sVec3_FP v = readSaturnVec3(ea);
    return v[0].asS32() == vec[0] &&
           v[1].asS32() == vec[1] &&
           v[2].asS32() == vec[2];
}


static void begin_azel_vdp1_frame()
{
    // initVDP1() owns the first six setup commands in context 0. Gameplay/UI
    // commands are emitted after them. Rewind only the transient tail each
    // frame, matching the Saturn command-list lifecycle without asking the
    // desktop renderer to flush it.
    auto& ctx = graphicEngineStatus.m14_vdp1Context[0];
    if (mainContextVdp1[0].size() < 1024)
        return;

    ctx.m0_currentVdp1WriteEA = mainContextVdp1[0].begin() + 6;
    ctx.m20_pCurrentVdp1Packet = ctx.m24_vdp1Packets;
    ctx.m1C = 0;
    ctx.mC = 0;
    ctx.m10 = ctx.m14[0].begin();
}

bool runtime_smoke_init()
{
    if (!saturn_memory_smoke_test()) {
        std::printf("[Azel] Saturn memory reader smoke test FAILED\n");
        lagi::platform::renderer::failure("[FAIL] SATURN MEMORY READERS");
        return false;
    }
    std::printf("[Azel] Saturn memory reader smoke test passed\n");
    lagi::platform::renderer::status("[PASS] SATURN MEMORY READERS", 0xFF40D0F0u);

    if (!lagi::disc::init()) {
        std::printf("[Disc] no valid ISO9660 image found in ux0:data/lagi\n");
        lagi::platform::renderer::failure("[FAIL] DISC 1 CUE/BIN MOUNT");
        return false;
    }
    std::printf("[Disc] mounted %s\n", lagi::disc::image_path());
    lagi::platform::renderer::status("[PASS] DISC 1 CUE/BIN + ISO9660", 0xFFF0C040u);

    initCommonFile();
    if (!gCommonFile ||
        gCommonFile->dragonLevelStats.size() != 9 ||
        SoundDataTable.size() != 79 ||
        gCommonFile->battleOverlaySetup.size() != 27 ||
        gCommonFile->battleActivationList.size() != 27) {
        std::printf("[Common] initialization FAILED\n");
        lagi::platform::renderer::failure("[FAIL] COMMON.DAT TABLES");
        return false;
    }

    std::printf("[Common] battle0: '%s' '%s' '%s' (%u sub-battles)\n",
                gCommonFile->battleOverlaySetup[0].m0_name.c_str(),
                gCommonFile->battleOverlaySetup[0].m4_prg.c_str(),
                gCommonFile->battleOverlaySetup[0].m8_fnt.c_str(),
                gCommonFile->battleOverlaySetup[0].mC_numSubBattles);

    lagi::platform::renderer::status("[PASS] COMMON.DAT DRAGON/BATTLE TABLES", 0xFFF08040u);
    lagi::platform::renderer::status("[PASS] SOUND TABLE 79/79", 0xFFE060E0u);

    if (!lagi::azel::init_direct_boot_target()) {
        std::printf("[DirectBoot] first 3D scene target resolution FAILED\n");
        lagi::platform::renderer::failure("[FAIL] DIRECT BOOT FIRST 3D TARGET");
        return false;
    }

    if (!lagi::azel::init_town_bootstrap()) {
        std::printf("[TownBoot] first 3D town overlay preflight FAILED\n");
        lagi::platform::renderer::failure("[FAIL] FIRST TOWN OVERLAY PREFLIGHT");
        return false;
    }

    if (!lagi::azel::init_town_runtime()) {
        std::printf("[TownRuntime] TWN_RUIN native scene owner FAILED\n");
        lagi::platform::renderer::failure("[FAIL] TWN_RUIN SCENE OWNER");
        return false;
    }

    // Decode the immutable Ruins cell/material set for Neptune. Runtime
    // transforms, camera, Edge and scripts remain owned by upstream Azel and
    // are copied across the platform boundary after each task frame.
    lagi::azel::StaticRoomDebugMesh townScene{};
    if (!lagi::azel::build_town_world_scene(townScene)) {
        std::printf("[TownRender] Ruins scene registration data FAILED\n");
        lagi::platform::renderer::failure("[FAIL] RUINS RENDER SCENE");
        return false;
    }

    lagi::azel::BasicWingDebugMesh edgeIdle{};
    if (!lagi::azel::build_edge_idle_debug_mesh(edgeIdle)) {
        std::printf("[Edge] idle model/textures reconstruction FAILED\n");
        lagi::platform::renderer::failure("[FAIL] EDGE IDLE MODEL");
        return false;
    }
    if (!lagi::platform::renderer::load_edge_idle_model(
            std::move(edgeIdle))) {
        std::printf("[Edge] renderer registration FAILED\n");
        lagi::platform::renderer::failure("[FAIL] EDGE IDLE RENDER MODEL");
        return false;
    }

    lagi::azel::BasicWingDebugMesh edgeShadow{};
    if (!lagi::azel::build_edge_shadow_debug_mesh(edgeShadow)) {
        std::printf("[Edge] shadow reconstruction FAILED\n");
        lagi::platform::renderer::failure("[FAIL] EDGE SHADOW MODEL");
        return false;
    }
    if (!lagi::platform::renderer::load_edge_shadow_model(
            std::move(edgeShadow))) {
        std::printf("[Edge] shadow renderer registration FAILED\n");
        lagi::platform::renderer::failure("[FAIL] EDGE SHADOW RENDER MODEL");
        return false;
    }

    if (!lagi::platform::renderer::load_static_room_viewer(townScene)) {
        std::printf("[TownRender] Neptune scene registration FAILED\n");
        lagi::platform::renderer::failure("[FAIL] RUINS RENDER REGISTRATION");
        return false;
    }

    if (!lagi::azel::load_dragon_common_data()) {
        std::printf("[Dragon] COMMON data initialization FAILED\n");
        lagi::platform::renderer::failure("[FAIL] DRAGON COMMON DATA");
        return false;
    }
    lagi::platform::renderer::status("[PASS] DRAGON COMMON DATA", 0xFF40E0A0u);

    unsigned basicWingBones = 0;
    unsigned basicWingHotpoints = 0;
    if (!lagi::azel::validate_basic_wing_hotpoints(&basicWingBones, &basicWingHotpoints)) {
        std::printf("[Dragon] Basic Wing hierarchy/hotpoint validation FAILED\n");
        lagi::platform::renderer::failure("[FAIL] DRAGON0 MCB / HOTPOINT DATA");
        return false;
    }

    char dragonStatus[78];
    std::snprintf(dragonStatus, sizeof(dragonStatus),
                  "[PASS] DRAGON0 MCB %u BONES / %u HOTPOINTS",
                  basicWingBones, basicWingHotpoints);
    lagi::platform::renderer::status(dragonStatus, 0xFF60D0FFu);

    unsigned dragonModels = 0;
    unsigned dragonVertices = 0;
    unsigned dragonPolygons = 0;
    if (!lagi::azel::validate_basic_wing_geometry(
            &dragonModels, &dragonVertices, &dragonPolygons)) {
        std::printf("[Dragon] Basic Wing geometry validation FAILED\n");
        lagi::platform::renderer::failure("[FAIL] DRAGON0 MODEL GEOMETRY");
        return false;
    }

    char geometryStatus[78];
    std::snprintf(geometryStatus, sizeof(geometryStatus),
                  "[PASS] DRAGON0 GEO %u MODELS / %u VERTS / %u POLYS",
                  dragonModels, dragonVertices, dragonPolygons);
    lagi::platform::renderer::status(geometryStatus, 0xFFC080FFu);

    if (!lagi::platform::renderer::load_basic_wing_viewer()) {
        std::printf("[GXM] Basic Wing viewer unavailable; continuing diagnostic runtime\n");
        lagi::platform::renderer::failure("[FAIL] GXM BASIC WING VIEWER");
    } else {
        lagi::platform::renderer::status("[PASS] GXM BASIC WING VIEWER READY", 0xFF80E0FFu);
    }

    lagi::platform::renderer::set_disc_alive(true);

    // Direct boot bypasses azelInit(), so restore the same physical-pad
    // state, action tables and VDP1 command context that normal Azel startup
    // establishes before gameplay begins.
    initSMPC();
    iniitInitialTaskStatsAndDebugSub();

    // The current upstream walk default reconstructs B/C as the LCS action
    // and A as run. The original manual documents A/C as LCS and B as
    // run/cancel. Keep the correction at the Lagi integration boundary until
    // the upstream table itself is corrected.
    const std::array<s32, 8> walkConfig = {
        0, 0, 1, 8, 4, 8, 6, 7
    };
    writeInputConfig(0, walkConfig, 0);

    // Town LCS/UI code writes authentic VDP1 commands even though Neptune,
    // rather than Azel's desktop backend, ultimately renders them.
    initVDP1();

    initHeap();

    // Direct boot also bypasses resetEngine()'s VDP2 initialization. Restore
    // Azel's real text-layer/font state before the town overlay loads
    // EVTRUIN.FNT so script, item and dialog text populate VDP2 VRAM exactly
    // as they do in the normal runtime.
    initVDP2();
    resetVdp2Strings();

    resetTasks();
    if (!start_twn_ruin_task_pipeline()) {
        lagi::platform::renderer::failure("[FAIL] TWN_RUIN TASK PIPELINE");
        return false;
    }
    lagi::platform::renderer::status("[PASS] TWN_RUIN TASK PIPELINE", 0xFF60A0F0u);
    std::printf("[Azel] TWN_RUIN native task pipeline started\n");

    // Hand presentation from the loading framebuffer to native GXM. The
    // legacy diagnostic screen remains hidden; Select is reserved for the
    // lightweight performance HUD.
    lagi::platform::renderer::show_town_scene();
    return true;
}

void runtime_smoke_frame()
{
    static unsigned int startupFrame = 0;
    const bool traceStartup = startupFrame < 3;
    if (traceStartup)
        lagi::platform::logging::writef(
            "[AzelFrame] %u begin tasks=%d\n",
            startupFrame, numActiveTask);

    // Present the Vita controls to Azel exactly as a Saturn/3D-pad
    // physical device. Azel then owns held/new-press state and translates the
    // physical bits through its on-foot/dragon/battle buttonConfig tables.
    auto& pending =
        graphicEngineStatus.m4514.m0_inputDevices[0].m16_pending;
    pending.m0_inputType = 2;
    pending.m6_buttonDown = lagi::platform::input::saturn_buttons_down();
    pending.m8_newButtonDown = lagi::platform::input::saturn_buttons_pressed();
    pending.mC_newButtonDown2 =
        lagi::platform::input::saturn_buttons_pressed();

    // Vita's stick axes are opposite Azel's 3D-pad convention on both
    // horizontal and vertical movement, so normalize them at the platform
    // boundary rather than changing upstream gameplay semantics.
    const float analogX = -lagi::platform::input::analog_x();
    const float analogY = -lagi::platform::input::analog_y();
    pending.m2_analogX = static_cast<s8>(
        analogX <= -1.0f ? -127 :
        analogX >= 1.0f ? 127 :
        analogX * 127.0f);
    pending.m3_analogY = static_cast<s8>(
        analogY <= -1.0f ? -127 :
        analogY >= 1.0f ? 127 :
        analogY * 127.0f);

    updateInputs();

    begin_azel_vdp1_frame();

    twn_ruin_frame_begin();
    lagi::azel_bridge::begin_frame();
    const std::uint64_t tasksStart = sceKernelGetProcessTimeWide();
    runTasks();
    twn_ruin_sync_platform_state();

    // Snapshot the Saturn VDP2 text plane at the same completed-task boundary
    // as the rest of the town presentation. Neptune consumes the snapshot
    // after the 3D/VDP1 scene, preserving Azel's ownership of all strings,
    // cursor placement, palettes and font/tile generation.
    lagi::platform::renderer::town_present_vdp2_text(
        getVdp2Vram(0),
        getVdp2Cram(0),
        getVdp2Vram(0x3E000));

    auto& vdp1Ctx = graphicEngineStatus.m14_vdp1Context[0];
    if (mainContextVdp1[0].size() >= 6) {
        const auto begin = mainContextVdp1[0].begin() + 6;
        const auto end = vdp1Ctx.m0_currentVdp1WriteEA;
        for (auto cmd = begin; cmd != end; ++cmd) {
            lagi::azel_bridge::Vdp1UiCommand ui{};
            ui.cmdCtrl = cmd->m0_CMDCTRL;
            ui.cmdPmod = cmd->m4_CMDPMOD;
            ui.cmdColr = cmd->m6_CMDCOLR;
            ui.cmdSrca = cmd->m8_CMDSRCA;
            ui.cmdSize = cmd->mA_CMDSIZE;
            ui.xa = cmd->mC_CMDXA;   ui.ya = cmd->mE_CMDYA;
            ui.xb = cmd->m10_CMDXB;  ui.yb = cmd->m12_CMDYB;
            ui.xc = cmd->m14_CMDXC;  ui.yc = cmd->m16_CMDYC;
            ui.xd = cmd->m18_CMDXD;  ui.yd = cmd->m1A_CMDYD;
            lagi::azel_bridge::record_vdp1_ui_command(ui);
        }
    }

    static unsigned int reportedVdp1Frames = 0;
    const unsigned int vdp1CommandCount =
        mainContextVdp1[0].size() >= 6
            ? static_cast<unsigned int>(
                vdp1Ctx.m0_currentVdp1WriteEA -
                (mainContextVdp1[0].begin() + 6))
            : 0u;
    static unsigned int reportedMultiVdp1Frames = 0;
    if (vdp1CommandCount > 1 && reportedMultiVdp1Frames < 12) {
        lagi::platform::logging::writef(
            "[VDP1UI-MULTI] frame=%u commands=%u\n",
            startupFrame, vdp1CommandCount);

        const auto begin = mainContextVdp1[0].begin() + 6;
        const auto end = vdp1Ctx.m0_currentVdp1WriteEA;
        unsigned int commandIndex = 0;
        for (auto cmd = begin; cmd != end; ++cmd, ++commandIndex) {
            lagi::platform::logging::writef(
                "[VDP1UI-MULTI] #%u CTRL=%04X PMOD=%04X COLR=%04X "
                "SRCA=%04X SIZE=%04X A=(%d,%d) B=(%d,%d) "
                "C=(%d,%d) D=(%d,%d)\n",
                commandIndex,
                static_cast<unsigned int>(cmd->m0_CMDCTRL),
                static_cast<unsigned int>(cmd->m4_CMDPMOD),
                static_cast<unsigned int>(cmd->m6_CMDCOLR),
                static_cast<unsigned int>(cmd->m8_CMDSRCA),
                static_cast<unsigned int>(cmd->mA_CMDSIZE),
                static_cast<int>(cmd->mC_CMDXA),
                static_cast<int>(cmd->mE_CMDYA),
                static_cast<int>(cmd->m10_CMDXB),
                static_cast<int>(cmd->m12_CMDYB),
                static_cast<int>(cmd->m14_CMDXC),
                static_cast<int>(cmd->m16_CMDYC),
                static_cast<int>(cmd->m18_CMDXD),
                static_cast<int>(cmd->m1A_CMDYD));
        }
        ++reportedMultiVdp1Frames;
    }

    if (vdp1CommandCount && reportedVdp1Frames < 8) {
        const auto& firstVdp1 = *(mainContextVdp1[0].begin() + 6);
        lagi::platform::logging::writef(
            "[VDP1UI] frame=%u commands=%u packets=%u "
            "CTRL=%04X PMOD=%04X COLR=%04X SRCA=%04X SIZE=%04X "
            "A=(%d,%d) B=(%d,%d) C=(%d,%d) D=(%d,%d)\n",
            startupFrame,
            vdp1CommandCount,
            static_cast<unsigned int>(
                vdp1Ctx.m20_pCurrentVdp1Packet -
                vdp1Ctx.m24_vdp1Packets),
            static_cast<unsigned int>(firstVdp1.m0_CMDCTRL),
            static_cast<unsigned int>(firstVdp1.m4_CMDPMOD),
            static_cast<unsigned int>(firstVdp1.m6_CMDCOLR),
            static_cast<unsigned int>(firstVdp1.m8_CMDSRCA),
            static_cast<unsigned int>(firstVdp1.mA_CMDSIZE),
            static_cast<int>(firstVdp1.mC_CMDXA),
            static_cast<int>(firstVdp1.mE_CMDYA),
            static_cast<int>(firstVdp1.m10_CMDXB),
            static_cast<int>(firstVdp1.m12_CMDYB),
            static_cast<int>(firstVdp1.m14_CMDXC),
            static_cast<int>(firstVdp1.m16_CMDYC),
            static_cast<int>(firstVdp1.m18_CMDXD),
            static_cast<int>(firstVdp1.m1A_CMDYD));
        ++reportedVdp1Frames;
    }

    const std::uint64_t tasksEnd = sceKernelGetProcessTimeWide();
    if (traceStartup)
        lagi::platform::logging::writef(
            "[AzelFrame] %u tasks complete us=%u tasks=%d\n",
            startupFrame,
            static_cast<unsigned int>(tasksEnd - tasksStart),
            numActiveTask);

    lagi::platform::renderer::town_profile_tasks_us(
        static_cast<unsigned int>(tasksEnd - tasksStart));

    // One queued frame: simulation may overlap the previous render, but the
    // producer cannot overwrite the published frame until the renderer is
    // finished with it. This bounds latency to a single frame.
    lagi::platform::renderer::town_wait_render_slot();
    lagi::azel_bridge::publish_frame();
    lagi::platform::renderer::town_publish_frame();
    if (traceStartup)
        lagi::platform::logging::writef(
            "[AzelFrame] %u publish complete\n", startupFrame);
    ++startupFrame;
    if (twn_ruin_task_pipeline_alive())
        lagi::platform::renderer::set_azel_alive(true);
}

} // namespace lagi::azel
