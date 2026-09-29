#include <cstdio>
#include "lagi/azel_compat.h"
#include "heap.h"
#include "task.h"
#include "VDP1.h"
#include "VDP2.h"
#include "common.h"
#include "lagi/platform.h"
#include "lagi/debug_mesh.h"
#include "lagi/azel_render_bridge.h"
#include "lagi/azel_direct_boot.h"
#include "lagi/azel_town_bootstrap.h"
#include "lagi/disc_image.h"
#include "commonOverlay.h"
#include "audio/soundDataTable.h"
#include "lagi/dragon_common.h"
#include <vector>

namespace lagi::azel {

struct SmokeTask final : s_workAreaTemplate<SmokeTask>
{
    int updates = 0;
    int draws = 0;

    static void Init(SmokeTask* self) { std::printf("[Azel] root task init\n"); self->updates = self->draws = 0; }
    static void UpdateTask(SmokeTask* self) { ++self->updates; }
    static void DrawTask(SmokeTask* self) { ++self->draws; }
    static void DeleteTask(SmokeTask*) { std::printf("[Azel] root task delete\n"); }

    static const TypedTaskDefinition* getTypedTaskDefinition()
    {
        static const TypedTaskDefinition def = { Init, UpdateTask, DrawTask, DeleteTask };
        return &def;
    }
};

static SmokeTask* gSmokeTask = nullptr;

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

    lagi::azel::StaticRoomDebugMesh firstRoom{};
    if (!lagi::azel::build_first_ruin_room_debug_mesh(firstRoom)) {
        std::printf("[RoomDebug] first ruin room reconstruction FAILED\n");
        lagi::platform::renderer::failure("[FAIL] FIRST RUIN ROOM GEOMETRY");
        return false;
    }

    if (!lagi::platform::renderer::load_static_room_viewer(firstRoom)) {
        std::printf("[RoomDebug] room viewer registration FAILED\n");
        lagi::platform::renderer::failure("[FAIL] RUIN ROOM VIEWER");
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

    initHeap();
    resetTasks();
    gSmokeTask = createRootTask<SmokeTask>();
    if (!gSmokeTask) {
        lagi::platform::renderer::failure("[FAIL] AZEL ROOT TASK CREATE");
        return false;
    }
    lagi::platform::renderer::status("[PASS] AZEL ROOT TASK CREATED", 0xFF60A0F0u);
    std::printf("[Azel] native task runtime linked; root=%p\n", static_cast<void*>(gSmokeTask));
    return true;
}

void runtime_smoke_frame()
{
    lagi::azel_bridge::begin_frame();
    runTasks();
    if (gSmokeTask && gSmokeTask->updates > 0 && gSmokeTask->draws > 0)
        lagi::platform::renderer::set_azel_alive(true);

    if (gSmokeTask && gSmokeTask->updates == 1)
        std::printf("[Azel] first task frame: update=%d draw=%d\n", gSmokeTask->updates, gSmokeTask->draws);
}

} // namespace lagi::azel
