#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/world_map_message.h"
#include "lagi/world_map_sprite_text.h"
#include "lagi/world_map_motion.h"

namespace lagi::worldmap {
void request_map_message(MapMessage& s, s8 mode)
{
    if (s.mode == 0) {s.mode=mode;s.queuedMode=-1;}
    else s.queuedMode=mode;
}
void release_map_message(MapMessage& s)
{
    if (s.child) {s.child->getTask()->markFinished();s.child=nullptr;}
}
bool update_map_message(MapMessage& s, s_workArea* parent, sSaturnPtr program,
                        sSaturnPtr targets, s32 count, s32 selected, bool confirmed)
{
    if (!parent || program.isNull() || targets.isNull() ||
        selected<0 || selected>=count) return false;
    switch(s.mode) {
    case 0:
        if (s.child) s.placement=project_map_label(readSaturnVec3(s.anchor),s.halfWidth);
        if (confirmed) {
            s.start=s.placement;s.factor=0;s.mode=1;s.queuedMode=-1;
        } else if (s.queuedMode>=0) {s.mode=s.queuedMode;s.queuedMode=-1;}
        break;
    case 1:
        if (s.child) {
            // WORLD 060575D0: interpolate from the captured screen position,
            // not last frame's result; preserve the raw depth bucket value.
            s.factor=map_approach(s.factor,0x10000,0x1999,0xF5C,0x28F);
            const s32 x=readSaturnS32(program+0x5C14)-s.halfWidth;
            const s32 y=readSaturnS32(program+0x5C18);
            s.placement.x=s32(u32(s.start.x)+u32(MTH_Mul(s.factor,s32(u32(x)-u32(s.start.x))).asS32()));
            s.placement.y=s32(u32(s.start.y)+u32(MTH_Mul(s.factor,s32(u32(y)-u32(s.start.y))).asS32()));
        }
        break;
    case 2: {
        if(s.storedTarget<0 || s.storedTarget>=count) return false;
        const auto entry=targets+s.storedTarget*0x24;
        auto name=readSaturnEA(entry+0x20);
        if(readSaturnS16(entry)==0x85 && !mainGameState.getBit(0x85)) name=program+0x5C1C;
        const s32 width=s32(u32(computeStringLength(name,999))<<3);
        s.halfWidth=s16(width/2);
        s.anchor=entry+8;
        s.child=create_map_sprite_text(parent,name,&s.placement);
        s.placement=project_map_label(readSaturnVec3(s.anchor),s.halfWidth);
        const s16 condition=readSaturnS16(entry);
        if(condition>0 && !mainGameState.getBit(condition)) {
            vdp2StringContext.m4_cursorX=vdp2StringContext.mC_X+2;
            vdp2StringContext.m8_cursorY=vdp2StringContext.m10_Y;
            VDP2DrawString(readSaturnString(program+0x5C38).c_str());
        }
        s.mode=0;
        break;
    }
    case 3:
        release_map_message(s);clearVdp2TextArea();s.storedTarget=s16(selected);s.mode=2;break;
    case 4: case 5:
        release_map_message(s);clearVdp2TextArea();
        vdp2StringContext.m4_cursorX=vdp2StringContext.mC_X+2;
        vdp2StringContext.m8_cursorY=vdp2StringContext.m10_Y;
        VDP2DrawString(readSaturnString(program+(s.mode==4?0x5C50:0x5C68)).c_str());
        s.mode=0;break;
    case 6:
        release_map_message(s);clearVdp2TextArea();s.mode=0;break;
    }
    return true;
}
}
