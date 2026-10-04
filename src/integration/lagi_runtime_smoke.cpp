#include <cstdio>
#include <psp2/kernel/processmgr.h>

#include "lagi/lagi_compat.h"
#include "lagi/platform.h"
#include "lagi/lagi_render_bridge.h"
#include "lagi/disc_image.h"
#include "lagi/lagi_azel_upstream_prelude.h"

#include "task.h"
#include "rootTask.h"
#include "VDP1.h"
#include "VDP2.h"
#include "movie/movie.h"
#include "titleScreen.h"
#include "kernel/moduleManager.h"

extern int numActiveTask;
void azelInit();
void resetEngine();
void updateFadeInterrupt();
void updateInputs();
p_workArea createTitleMenuTask(p_workArea);

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

    if (!lagi::disc::init()) {
        std::printf("[Disc] no valid ISO9660 image found in ux0:data/lagi\n");
        lagi::platform::renderer::failure("[FAIL] DISC 1 CUE/BIN MOUNT");
        return false;
    }

    lagi::platform::renderer::set_disc_alive(true);

    // 0.040-alpha: enter through Azel's native startup path. Azel owns all
    // game/global initialization, VDP state, initial task creation, title
    // sequencing, New Game setup and subsequent game-status transitions.
    // Lagi provides only the Vita platform services underneath those calls.
    azelInit();
    resetEngine();

    // Native boot can enter the title movie immediately. Stop the CPU loading
    // framebuffer from overwriting GXM/movie presentation; front-end VDP2
    // composition will be added to this same native presentation path.
    lagi::platform::renderer::show_town_scene();

    lagi::platform::logging::writef(
        "[AzelBoot] native azelInit/resetEngine complete; initial task active\n");
    return true;
}
void runtime_smoke_frame()
{
    static unsigned int startupFrame = 0;
    const bool traceStartup = startupFrame < 3;

    // Present Vita controls as Azel's Saturn 3D pad. Input translation stays a
    // platform responsibility; task/menu/gameplay interpretation stays Azel's.
    auto& pending =
        graphicEngineStatus.m4514.m0_inputDevices[0].m16_pending;
    pending.m0_inputType = 2;
    pending.m6_buttonDown = lagi::platform::input::saturn_buttons_down();
    pending.m8_newButtonDown = lagi::platform::input::saturn_buttons_pressed();
    pending.mC_newButtonDown2 =
        lagi::platform::input::saturn_buttons_pressed();

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

    // Azel's Saturn VBlank normally advances fade state before the task pass.
    updateFadeInterrupt();
    lagi::platform::renderer::set_azel_fade_color(
        vdp2Controls.m20_registers[0].m114_COAR,
        vdp2Controls.m20_registers[0].m116_COAG,
        vdp2Controls.m20_registers[0].m118_COAB);

    begin_azel_vdp1_frame();
    lagi::azel_bridge::begin_frame();

    if (traceStartup)
        lagi::platform::logging::writef(
            "[AzelBoot] frame=%u tasks=%d currentInitial=%p pendingInitial=%p\n",
            startupFrame,
            numActiveTask,
            reinterpret_cast<void*>(initialTaskStatus.m_currentTask),
            reinterpret_cast<void*>(initialTaskStatus.m_pendingTask));

    runTasks();

    // Service the Saturn-side VDP2 deferred register/DMA work and the platform
    // movie backend at the same host-frame boundary used by the existing
    // native runtime integration.
    interruptVDP2Update();
    lastUpdateFunction();

    // Front-end VDP2 presentation is a platform service. Azel owns all title
    // graphics, text, palettes, blinking, input and state transitions; Lagi
    // simply presents the VDP2 memory that Azel has already produced.
    const bool titleActive =
        initialTaskStatus.m_currentTask == createTitleScreenTask ||
        initialTaskStatus.m_currentTask == createTitleMenuTask;
    // FLD_D5 creates the name-entry UI while its opening field script is
    // still running, then immediately disables NBG0 and pauses the UI. Azel
    // later calls nameEntryEnable() when that script completes. Respect that
    // native layer ownership instead of exposing the staged keyboard merely
    // because gameStatus has already reached 2.
    const bool nameEntryActive =
        gGameStatus.m4_gameStatus == 2 &&
        (vdp2Controls.m4_pendingVdp2Regs->m20_BGON & 0x1) != 0;
    if (titleActive || nameEntryActive) {
        // Azel owns the VDP2 memory/register state. Neptune uploads the raw
        // VRAM/CRAM and interprets the active front-end layers directly on
        // SGX.  FLD_D5 scrolls its 2-plane keyboard map to change pages.
        lagi::platform::renderer::frontend_present_vdp2(
            getVdp2Vram(0),
            getVdp2Cram(0),
            nameEntryActive ? 1u : 0u,
            nameEntryActive
                ? (vdp2Controls.m20_registers[0].m70_SCXN0 >> 16)
                : 0,
            nameEntryActive
                ? (vdp2Controls.m20_registers[0].m74_SCYN0 >> 16)
                : 0);
    } else if (gGameStatus.m0_gameMode == 0 &&
               fileInfoStruct.mC_gfsHandle == nullptr) {
        // The native movie task closes its stream at the start of the exit
        // fade.  Re-submit the retained final frame so Azel's live color
        // offset continues to reach the display until the next module owns it.
        lagi::platform::renderer::movie_republish_frame();
    }

    ++startupFrame;
}

} // namespace lagi::azel
