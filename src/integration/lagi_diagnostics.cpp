#include "lagi/lagi_diagnostics.h"

#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/platform.h"

#include "VDP2.h"
#include "kernel/fade.h"
#include "kernel/moduleManager.h"

namespace lagi::diagnostics {

bool saturn_memory_readers()
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

    const s32 vec[3] = {0x00010000, -0x00020000, 0x00008000};
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

void log_title_vdp2_once()
{
    static bool logged = false;
    if (logged)
        return;

    unsigned int banks[8] = {};
    unsigned int nonzeroPatterns = 0;
    for (unsigned int page = 0; page < 2; ++page) {
        const unsigned int base = 0x10000u + page * 0x800u;
        for (unsigned int i = 0; i < 0x400u; ++i) {
            const u16 pattern = getVdp2VramU16(base + i * 2u);
            if (pattern != 0)
                ++nonzeroPatterns;
            ++banks[(pattern >> 12) & 7u];
        }
    }

    if (nonzeroPatterns == 0)
        return;

    platform::logging::writef(
        "[VDP2TitleDiag] patterns=%u banks=%u,%u,%u,%u,%u,%u,%u,%u "
        "cram0=%04X cram1=%04X cram2=%04X cram3=%04X\n",
        nonzeroPatterns,
        banks[0], banks[1], banks[2], banks[3],
        banks[4], banks[5], banks[6], banks[7],
        getVdp2CramU16(0),
        getVdp2CramU16(2),
        getVdp2CramU16(4),
        getVdp2CramU16(6));
    logged = true;
}

void trace_fade_state()
{
    static bool previousFade0Stopped = true;
    static bool previousFade1Stopped = true;
    static unsigned int previousFade0Counter = ~0u;

    const bool fade0Stopped = g_fadeControls.m0_fade0.m20_stopped != 0;
    const bool fade1Stopped = g_fadeControls.m24_fade1.m20_stopped != 0;
    const unsigned int fade0Counter =
        static_cast<unsigned int>(g_fadeControls.m0_fade0.m1E_counter);
    const bool progressSample =
        !fade0Stopped &&
        fade0Counter != previousFade0Counter &&
        ((fade0Counter % 10u) == 0u);

    if (fade0Stopped != previousFade0Stopped ||
        fade1Stopped != previousFade1Stopped ||
        progressSample) {
        const auto& regs = vdp2Controls.m20_registers[0];
        platform::logging::writef(
            "[AzelFade] f0stop=%u count=%u f1stop=%u "
            "CLOFEN=%04X CLOFSL=%04X "
            "A=(%d,%d,%d) B=(%d,%d,%d) status=%02X mode=%u\n",
            fade0Stopped ? 1u : 0u,
            fade0Counter,
            fade1Stopped ? 1u : 0u,
            regs.m110_CLOFEN,
            regs.m112_CLOFSL,
            static_cast<int>(regs.m114_COAR),
            static_cast<int>(regs.m116_COAG),
            static_cast<int>(regs.m118_COAB),
            static_cast<int>(regs.m11A_COBR),
            static_cast<int>(regs.m11C_COBG),
            static_cast<int>(regs.m11E_COBB),
            static_cast<unsigned int>(gGameStatus.m4_gameStatus),
            static_cast<unsigned int>(gGameStatus.m0_gameMode));
    }

    previousFade0Stopped = fade0Stopped;
    previousFade1Stopped = fade1Stopped;
    previousFade0Counter = fade0Counter;
}

} // namespace lagi::diagnostics
