// MAIN 060231A8 reads 0x1C-byte sprite records from COMMON 00210768.
#include "commonOverlay.h"
// Entries 0/1/2/3 are distinct resources; do not mask the index to one bit.
static s_nameEntrySpriteData nameEntryNativeCursor(u32 index)
{
    const auto record=gCommonFile->getSaturnPtr(0x00210768+index*0x1c);
    s_nameEntrySpriteData sprite{};
    sprite.m0_CMDCTRL=readSaturnU16(record+2);
    sprite.m2_CMDPMOD=readSaturnU16(record+4);
    sprite.m4_CMDSRCAbase=readSaturnU16(record+6);
    sprite.m6_CMDSIZE=readSaturnU16(record+8);
    sprite.m8_CMDCOLR=readSaturnU16(record+10);
    sprite.mC_widthFP=readSaturnS32(record+12);
    sprite.m10_heightFP=readSaturnS32(record+16);
    sprite.m14_xOffsetFP=readSaturnS32(record+20);
    sprite.m18_yOffsetFP=readSaturnS32(record+24);
    return sprite;
}
