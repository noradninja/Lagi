#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/battle_damage_digits.h"
#include "battle/battleGenericData.h"

// Original BTL_A3 0605BCDC, called by damageDisplay_draw at 0605C0DE.
// Read the authored sprite/color tables; do not embed game font data.
void lagiDrawBattleDamageDigits(const char* digits, s16 x, s16 y,
    s16 width, s16 height, u16 vdp1Base, u16 colorType, s32 mode)
{
    if (!digits || !g_BTL_GenericData || mode < 0 || mode > 1) return;
    auto& context = graphicEngineStatus.m14_vdp1Context[0];
    const auto table = g_BTL_GenericData->getSaturnPtr(mode ? 0x060B1F04 : 0x060B1F7C);
    const auto colors = g_BTL_GenericData->getSaturnPtr(0x060B1FF4 + colorType * 8);
    for (; *digits; ++digits) {
        const int digit = *digits - '0';
        if (digit < 0 || digit > 9) return;
        const auto sprite = table + digit * 0xC;
        auto& command = *context.m0_currentVdp1WriteEA;
        command.m0_CMDCTRL = 0x1001;
        command.m4_CMDPMOD = 0x48C;
        command.m6_CMDCOLR = readSaturnU16(sprite + 6) + vdp1Base;
        command.m8_CMDSRCA = readSaturnU16(sprite + 2) + vdp1Base;
        command.mA_CMDSIZE = readSaturnU16(sprite + 8);
        command.mC_CMDXA = x;
        command.mE_CMDYA = -y;
        command.m14_CMDXC = x + width - 1;
        command.m16_CMDYC = height - y - 1;
        command.m1C_CMDGRA = static_cast<u16>(context.m10 - context.m14[0].begin());
        auto& output = *context.m10++;
        for (unsigned vertex = 0; vertex < 4; ++vertex)
            output[vertex] = readSaturnU16(colors + vertex * 2);
        context.m20_pCurrentVdp1Packet->m4_bucketTypes = 0;
        context.m20_pCurrentVdp1Packet->m6_vdp1EA = &command;
        ++context.m20_pCurrentVdp1Packet;
        ++context.m0_currentVdp1WriteEA;
        ++context.m1C;
        ++context.mC;
        x += width;
    }
}
