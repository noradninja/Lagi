#include <cstdio>
#include "lagi/azel_compat.h"
#include "heap.h"
#include "task.h"
#include "VDP1.h"
#include "VDP2.h"
#include "common.h"
#include "lagi/platform.h"
#include "lagi/disc_image.h"
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


static bool validate_common_dat(sSaturnMemoryFile& common)
{
    // Azel commonOverlay.cpp: 9-entry dragon level stat pointer table.
    const sSaturnPtr dragonTable = common.getSaturnPtr(0x00206FF8);
    for (int i = 0; i < 9; ++i) {
        const sSaturnPtr stat = readSaturnEA(dragonTable + i * 4);
        const u32 addr = static_cast<u32>(stat.m_offset);
        if (addr < common.m_base || addr + 0x1E > common.m_base + common.m_dataSize)
            return false;

        volatile s8 first = readSaturnS8(stat);
        volatile s8 last = readSaturnS8(stat + 0x1D);
        (void)first;
        (void)last;
    }

    // Azel commonOverlay.cpp: first battle overlay descriptor.
    const sSaturnPtr battle = common.getSaturnPtr(0x002005DC);
    const sSaturnPtr nameEA = readSaturnEA(battle + 0x0);
    const sSaturnPtr prgEA  = readSaturnEA(battle + 0x4);
    const sSaturnPtr fntEA  = readSaturnEA(battle + 0x8);

    const std::string name = readSaturnString(nameEA);
    const std::string prg  = readSaturnString(prgEA);
    const std::string fnt  = readSaturnString(fntEA);

    if (name.empty() || prg.empty() || fnt.empty())
        return false;

    std::printf("[Disc] COMMON validated: dragon stats + battle0 '%s' '%s' '%s'\n",
                name.c_str(), prg.c_str(), fnt.c_str());
    return true;
}

bool runtime_smoke_init()
{
    if (!saturn_memory_smoke_test()) {
        std::printf("[Azel] Saturn memory reader smoke test FAILED\n");
        return false;
    }
    std::printf("[Azel] Saturn memory reader smoke test passed\n");

    if (!lagi::disc::init()) {
        std::printf("[Disc] no valid ISO9660 image found in ux0:data/lagi\n");
        return false;
    }
    std::printf("[Disc] mounted %s\n", lagi::disc::image_path());

    std::vector<u8> commonData;
    if (!lagi::disc::read_file("COMMON.DAT", commonData) || commonData.size() < 16) {
        std::printf("[Disc] COMMON.DAT not found or unreadable\n");
        return false;
    }

    sSaturnMemoryFile common;
    common.m_name = "COMMON.DAT";
    common.m_data = commonData.data();
    common.m_dataSize = static_cast<u32>(commonData.size());
    common.m_base = 0x00200000;

    const sSaturnPtr commonRoot = common.getSaturnPtr(common.m_base);
    volatile u32 probe0 = readSaturnU32(commonRoot);
    volatile u16 probe4 = readSaturnU16(commonRoot + 4);
    (void)probe0;
    (void)probe4;

    std::printf("[Disc] COMMON.DAT loaded: %u bytes, first=%08X %04X\n",
                common.m_dataSize, probe0, probe4);

    if (!validate_common_dat(common)) {
        std::printf("[Disc] COMMON.DAT structural validation FAILED\n");
        return false;
    }

    lagi::platform::renderer::set_disc_alive(true);

    initHeap();
    resetTasks();
    gSmokeTask = createRootTask<SmokeTask>();
    if (!gSmokeTask) return false;
    std::printf("[Azel] native task runtime linked; root=%p\n", static_cast<void*>(gSmokeTask));
    return true;
}

void runtime_smoke_frame()
{
    runTasks();
    if (gSmokeTask && gSmokeTask->updates > 0 && gSmokeTask->draws > 0)
        lagi::platform::renderer::set_azel_alive(true);

    if (gSmokeTask && gSmokeTask->updates == 1)
        std::printf("[Azel] first task frame: update=%d draw=%d\n", gSmokeTask->updates, gSmokeTask->draws);
}

} // namespace lagi::azel
