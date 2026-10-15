#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/azel_map_menu.h"
#include "commonOverlay.h"
#include "field/fieldDragon.h"

bool lagiAzelResolveCurrentMapContext(LagiAzelMapContext& out)
{
    out = {};
    // MAIN 06016C64 reads the original two-bit scene class, not menu ID.
    if (mainGameState.readPackedBits(0,2) != 0 || !gCommonFile || !gCommonFile->m_data)
        return false;
    const auto* field = getFieldTaskPtr();
    if (!field || !field->m8_pSubFieldData ||
        !field->m8_pSubFieldData->m338_pDragonTask)
        return false;
    const int fieldIndex = field->m2C_currentFieldIndex;
    int subfield = field->m2E_currentSubFieldIndex;
    if (fieldIndex < 0 || fieldIndex >= 23 || subfield < 0)
        return false;
    const auto* dragon = field->m8_pSubFieldData->m338_pDragonTask;
    sVec3_FP position = dragon->m8_pos;
    s32 heading = dragon->m20_angle[1].asS32();

    if (fieldIndex == 0xF || fieldIndex == 0x12) {
        // Native 0602F8E4 returns the zone from 0602F87C as well as writing
        // transformed XYZ. Read complete live COMMON tables, not upstream's
        // truncated sixteen-entry copies (native callers also use 17/1A).
        const auto range = [&](std::uint32_t address,std::size_t length) {
            return address >= gCommonFile->m_base &&
                std::size_t(address-gCommonFile->m_base) <= gCommonFile->m_dataSize &&
                length <= gCommonFile->m_dataSize-std::size_t(address-gCommonFile->m_base);
        };
        const auto at = [&](std::uint32_t address) {
            return gCommonFile->getSaturnPtr(address);
        };
        // The byte lookup ends where its adjacent zone-offset table begins.
        if (subfield >= 0x24 || !range(0x2161F4+subfield,1) ||
            !range(0x216290+subfield*8,8))
            return false;
        const int originalSubfield = subfield;
        const unsigned zone = readSaturnU8(at(0x2161F4+subfield));
        const auto offsets = at(0x216290+subfield*8);
        const auto add = [](s32 a,s32 b) {
            return static_cast<s32>(static_cast<u32>(a)+static_cast<u32>(b));
        };
        position[0] = add(position[0].asS32(),readSaturnS32(offsets));
        if (zone >= 0xF) {
            position[2] = static_cast<s32>(0u-static_cast<u32>(position[1].asS32())-
                static_cast<u32>(readSaturnS32(offsets+4)));
            subfield = 0xF;
        } else {
            if (!range(0x216218+zone*8,8)) return false;
            const auto zoneOffsets = at(0x216218+zone*8);
            position[0] = add(position[0].asS32(),readSaturnS32(zoneOffsets));
            position[2] = add(add(position[2].asS32(),readSaturnS32(offsets+4)),
                readSaturnS32(zoneOffsets+4));
            subfield = static_cast<int>(zone);
        }
        if (originalSubfield == 0 || originalSubfield == 6 || originalSubfield == 9 ||
            originalSubfield == 0x17 || originalSubfield == 0x1A) {
            // 06016CBC..CE0: Y(yaw), then X(pitch), then native atan2.
            sMatrix4x3 matrix;
            initMatrixToIdentity(&matrix);
            rotateMatrixShiftedY(dragon->m20_angle[1],&matrix);
            rotateMatrixShiftedX(dragon->m20_angle[0],&matrix);
            heading = atan2_FP(matrix.m[0][2].asS32(),matrix.m[1][2].asS32());
        }
    }

    LagiAzelMapContext context;
    // The selected slot comes from authoritative live field state/native zone
    // lookup, not an arbitrary UI index or a guessed subfield count. The slot
    // reader still bounds every COMMON address and filename independently.
    if (gCommonFile->m_base != 0x200000 ||
        !lagiAzelReadFieldMapDescriptorSlot(gCommonFile->m_data,gCommonFile->m_dataSize,
            static_cast<unsigned>(fieldIndex),static_cast<unsigned>(subfield),context.descriptor))
        return false;
    context.field = fieldIndex;
    context.subfield = subfield;
    context.x = static_cast<std::int16_t>(intDivide(0x10000,position[0].asS32()));
    context.y = static_cast<std::int16_t>(intDivide(0x10000,position[2].asS32())+0x400);
    context.heading = heading;
    out = context;
    return true;
}
